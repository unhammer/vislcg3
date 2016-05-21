/*
 * Copyright (C) 2016, Kevin Brubeck Unhammer <unhammer@fsfe.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mwesplit.hpp"

namespace CG3 {


URegularExpression *CG_LINE = uregex_openC
	("^"
	 "(\"<(.*)>\".*" // wordform, group 2
	 "|([\t ]+)(\"[^\"]*\"\\S*)(\\s+\\S+)*.*" // reading, group 3, 4, 5
	 "|(;[\t ]+.*)" // removed reading, group 6
	 ")",
	 0, NULL, NULL);

URegularExpression *CG_WFTAG = uregex_openC
	(".*(\\s+\"<(.*)>\").*",
	 0, NULL, NULL);

struct Reading {
		UString ana;
		UString wftag;
};

struct Cohort {
		UString form;
		std::vector<std::vector<Reading> > readings;
		std::string postblank;
};

const std::pair<UString, UString> extr_wftag(const UString line)
{
	UErrorCode status = U_ZERO_ERROR;
	uregex_setText(CG_WFTAG, line.c_str(), line.length(), &status);

	if (uregex_find(CG_WFTAG, -1, &status)) { // TODO: need both find and groupCount?
		int32_t gc = uregex_groupCount(CG_WFTAG, &status);
		if(gc == 2) {
			int32_t start = uregex_start(CG_WFTAG, 1, &status);
			int32_t end = uregex_end(CG_WFTAG, 1, &status);
			const UString rest = line.substr(0, start) + line.substr(end);
			UChar wftag[1024];
			int32_t len = uregex_group(CG_WFTAG, 2, wftag, 1024, &status);
			return std::make_pair(rest, UString(wftag, len));
		}
		else {
			return std::make_pair(line, "");	// TODO: what if there's an empty wftag? call it a bug?
		}
	}
	else {
		return std::make_pair(line, "");	// TODO: what if there's an empty wftag? call it a bug?
	}
}

void print_cohort(UFILE *output, const Cohort& c) {
	u_fprintf(output, "\"<%S>\"\n", c.form.c_str());
	std::set<std::string> seen;
	foreach (r, c.readings) {
		std::ostringstream subs;
		foreach(s, *r) {
			subs << s->ana << std::endl;
		}
		std::string sub = subs.str();
		if(seen.count(sub) == 0) {
			u_fprintf(output, "%S", sub.c_str());
			seen.insert(sub);
		}
	}
	if(!c.postblank.empty()) {
		u_fprintf(output, ":%S\n", c.postblank.c_str());
	}
}

const std::string reindent(const std::string ana, const size_t level) {
	std::string indent = std::string(level+1, '\t');
	return indent + ana.substr(ana.find("\""));
}


const Cohort cohort_from_wftag(const std::string form) {
	// Treats any trailing blanks as if they should be in between
	// words (TODO: only do this for non-final words?)
	size_t i = form.find_last_not_of(" \n\r\t")+1;
	return (Cohort) { form.substr(0, i),
			std::vector<std::vector<Reading> >(),
			form.substr(i) };
}

const std::vector<Cohort> split_cohort(const Cohort& mwe, const unsigned int lno) {
	size_t n_wftags = 0;
	foreach(r, mwe.readings) {
		if(!r->empty() && !r->front().wftag.empty()) {
			++n_wftags;
		}
	}
	if(n_wftags < mwe.readings.size()) {
		if(n_wftags > 0) {
			std::cerr << "WARNING: Line " << lno << ": Some but not all main-readings of \"<" << mwe.form << ">\" had wordform-tags (not completely mwe-disambiguated?), not splitting."<< std::endl;
		}
		return std::vector<Cohort>(1, mwe);
	}

	std::vector<Cohort> cos;
	foreach(r, mwe.readings) {
		size_t pos = -1;
		foreach(s, *r) {
			if(!(s->wftag.empty())) {
				++pos;
				Cohort c = cohort_from_wftag(s->wftag);
				while(cos.size() < pos+1) {
					cos.push_back(c);
				}
				if(cos[pos].form != c.form) {
					std::cerr << "WARNING: Line " << lno << ": Ambiguous word form tags for same cohort, '" << cos[pos].form << "' vs '" << s->wftag << "'"<< std::endl;
				}
				cos[pos].readings.push_back(std::vector<Reading>());
			}
			size_t level = cos[pos].readings.back().size();
			Reading n = { reindent(s->ana, level), "" };
			cos[pos].readings.back().push_back(n);
		}
	}
	// The last word forms are the top readings:
	std::reverse(cos.begin(), cos.end());
	return cos;
}

void split_and_print(UFILE *output, const Cohort& c, const unsigned int lno) {
	if(!c.form.empty()) {
		const std::vector<Cohort> cos = split_cohort(c, lno);
		foreach(c, cos) {
			print_cohort(output, *c);
		}
	}
}


void mwesplit(istream& input, UFILE *output, UFILE *ux_stderr)
{
	if (!input.good()) {
		u_fprintf(ux_stderr, "Error: Input is null - nothing to parse!\n");
		CG3Quit(1);
	}
	if (input.eof()) {
		u_fprintf(ux_stderr, "Error: Input is empty - nothing to parse!\n");
		CG3Quit(1);
	}
	if (!output) {
		u_fprintf(ux_stderr, "Error: Output is null - cannot write to nothing!\n");
		CG3Quit(1);
	}
	std::vector<UChar> line(1024, 0);
	std::vector<UChar> cleaned(line.size(), 0);
	bool ignoreinput = false;

	Cohort cohort = (Cohort) { UString(), std::vector<std::vector<Reading> >(), "" };
	std::vector<std::pair<size_t, Reading*> > indents;

	size_t indentation = 0;
	uint32_t numLines = 0;

	while (!input.eof()) {
		size_t offset = 0, packoff = 0;
		// Read as much of the next line as will fit in the current buffer
		while (input.gets(&line[offset], line.size() - offset - 1)) {
			// Copy the segment just read to cleaned
			for (size_t i = offset; i < line.size(); ++i) {
				// Only copy one space character, regardless of how many are in input
				if (ISSPACE(line[i]) && !ISNL(line[i])) {
					cleaned[packoff++] = ' ';
					while (ISSPACE(line[i]) && !ISNL(line[i])) {
						++i;
					}
				}
				// Break if there is a newline
				if (ISNL(line[i])) {
					cleaned[packoff + 1] = cleaned[packoff] = 0;
					goto gotaline; // Oh how I wish C++ had break 2;
				}
				if (line[i] == 0) {
					cleaned[packoff + 1] = cleaned[packoff] = 0;
					break;
				}
				cleaned[packoff++] = line[i];
			}
			// If we reached this, buffer wasn't big enough. Double the size of the buffer and try again.
			offset = line.size() - 2;
			line.resize(line.size() * 2, 0);
			cleaned.resize(line.size() + 1, 0);
		}

	gotaline:
		// Trim trailing whitespace
		while (cleaned[0] && ISSPACE(cleaned[packoff - 1])) {
			cleaned[packoff - 1] = 0;
			--packoff;
		}
		if (!ignoreinput && cleaned[0] == '"' && cleaned[1] == '<') {
			UChar *space = &cleaned[0];
			if (space[0] == '"' && space[1] == '<') {
				++space;
				SKIPTO_NOSPAN(space, '"');
				while (*space && space[-1] != '>') {
					++space;
					SKIPTO_NOSPAN(space, '"');
				}
				SKIPTOWS(space, 0, true, true);
				--space;
			}
			if (space[0] != '"' || space[-1] != '>') {
				u_fprintf(ux_stderr, "Warning: %S on line %u looked like a cohort but wasn't - treated as text.\n", &cleaned[0], numLines);
				u_fflush(ux_stderr);
				goto istext;
			}
			space[1] = 0;
			split_and_print(output, cohort, numLines);
			cohort = (Cohort) {
				&cleaned[0],
				std::vector<std::vector<Reading> >(),
				""
			};
			indentation = 0;
			indents.clear();
		}
		else if (cleaned[0] == ' ' && cleaned[1] == '"' // && cCohort
			) {
			// Count current indent level
			size_t indent = 0;
			while (ISSPACE(line[indent])) {
				++indent;
			}
			while (!indents.empty() && indent <= indents.back().first) {
				indents.pop_back();
			}
			if (!indents.empty() && indent > indents.back().first) {
				if (indents.back().second->next) {
					u_fprintf(ux_stderr, "Warning: Sub-reading %S on line %u will be ignored and lost as each reading currently only can have one sub-reading.\n", &cleaned[0], numLines);
					u_fflush(ux_stderr);
					continue;
				}
			}
			else {
				cReading = alloc_reading(cCohort);
			}

			UChar *space = &cleaned[1];
			UChar *base = space;
			if (*space == '"') {
				++space;
				SKIPTO_NOSPAN(space, '"');
				SKIPTOWS(space, 0, true, true);
				--space;
			}

			std::pair<UString, UString> ana_wf = extr_wftag(&cleaned[0]);
			Reading r = { ana_wf.first, ana_wf.second };
			if(cohort.readings.empty()) {
				cohort.readings.push_back(std::vector<Reading>(1, r));
			}
			else if((unsigned)result[3].length() > indentation) {
				// we know readings non-empty because above if:
				cohort.readings.back().push_back(r);
			}
			else {
				// indentation same or decreased (CG doesn't allow "ambiguous" subreadings
				// of a reading, so we always go all the way back up to main cohort here)
				cohort.readings.push_back(std::vector<Reading>(1, r));
			}
			indentation = result[3].length();
		}
		else {
			if (!ignoreinput && cleaned[0] == ' ' && cleaned[1] == '"') {
				if (true // verbosity_level > 0
					) {
					u_fprintf(ux_stderr, "Warning: %S on line %u looked like a reading but there was no containing cohort - treated as plain text.\n", &cleaned[0], numLines);
					u_fflush(ux_stderr);
				}
			}
		istext:
			if (cleaned[0]) {
				if (u_strcmp(&cleaned[0], stringbits[S_CMD_FLUSH].getTerminatedBuffer()) == 0) {
					u_fprintf(ux_stderr, "Info: FLUSH encountered on line %u. Flushing...\n", numLines);
					split_and_print(output, cohort, numLines);
					cohort = (Cohort) {
						&cleaned[0],
						std::vector<std::vector<Reading> >(),
						""
					};
					indentation = 0;
					u_fprintf(output, "%S", &line[0]);
					line[0] = 0;
					u_fflush(output);
					u_fflush(ux_stderr);
					fflush(stdout);
					fflush(stderr);
				}
				else if (u_strcmp(&cleaned[0], stringbits[S_CMD_IGNORE].getTerminatedBuffer()) == 0) {
					u_fprintf(ux_stderr, "Info: IGNORE encountered on line %u. Passing through all input...\n", numLines);
					ignoreinput = true;
				}
				else if (u_strcmp(&cleaned[0], stringbits[S_CMD_RESUME].getTerminatedBuffer()) == 0) {
					u_fprintf(ux_stderr, "Info: RESUME encountered on line %u. Resuming CG...\n", numLines);
					ignoreinput = false;
				}
				else if (u_strcmp(&cleaned[0], stringbits[S_CMD_EXIT].getTerminatedBuffer()) == 0) {
					u_fprintf(ux_stderr, "Info: EXIT encountered on line %u. Exiting...\n", numLines);
					u_fprintf(output, "%S", &line[0]);
					goto CGCMD_EXIT;
				}
				// ignore SETVAR, REMVAR

				if (line[0]) {
					u_fprintf(output, "%S", &line[0]);
				}
			}
		}
		numLines++;
		line[0] = cleaned[0] = 0;
	}
	for (UString line; // std::getline(input, line)
		     ;) {
		// UErrorCode status = U_ZERO_ERROR;
		// uregex_setText(CG_LINE, line.c_str(), line.length(), &status);

		// std::match_results<const char*> result;
		// std::regex_match(line.c_str(), result, CG_LINE);
		// if(!result.empty() && result[2].length() != 0) {
		// 	split_and_print(output, cohort, lines);
		// 	cohort = { result[2], {}, "" };
		// 	indentation = 0;
		// }
		// else
			if(!result.empty() && result[3].length() != 0) {
			auto ana_wf = extr_wftag(line);
			Reading r = { ana_wf.first, ana_wf.second };
			if(cohort.readings.empty()) {
				cohort.readings.push_back({r});
			}
			else if((unsigned)result[3].length() > indentation) {
				// we know readings non-empty because above if:
				cohort.readings.back().push_back(r);
			}
			else {
				// indentation same or decreased (CG doesn't allow "ambiguous" subreadings
				// of a reading, so we always go all the way back up to main cohort here)
				cohort.readings.push_back({r});
			}
			indentation = result[3].length();
		}
		else if(!result.empty() && result[6].length() != 0) {
			// We just skip removed readings; don't output them.
			// (Might have to output commented word forms then ...)
		}
		else {
			split_and_print(output, cohort, numLines);
			cohort = { "", {} };
			indentation = 0;
			output << line << std::endl;
		}
	}

	split_and_print(output, cohort, numLines);
CGCMD_EXIT:
}
}
