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
		std::string ana;
		std::string wftag;
};

struct Cohort {
		std::string form;
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
		for(std::vector<Reading>::const_iterator s = r->begin(); s != r->end(); s++) {
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
	for(std::vector<std::vector<Reading> >::const_iterator r = mwe.readings.begin();
	    r != mwe.readings.end(); r++) {
		if(!r->empty() && !r->front().wftag.empty()) {
			++n_wftags;
		}
	}
	if(n_wftags < mwe.readings.size()) {
		if(n_wftags > 0) {
			std::cerr << "WARNING: Line " << lno << ": Some but not all main-readings of \"<" << mwe.form << ">\" had wordform-tags (not completely mwe-disambiguated?), not splitting."<< std::endl;
		}
		return (std::vector<Cohort>){ mwe };
	}

	std::vector<Cohort> cos;
	foreach(r, mwe.readings) {
		size_t pos = -1;
		foreach (s, r) {
			if(!s.wftag.empty()) {
				++pos;
				Cohort c = cohort_from_wftag(s.wftag);
				while(cos.size() < pos+1) {
					cos.push_back(c);
				}
				if(cos[pos].form != c.form) {
					std::cerr << "WARNING: Line " << lno << ": Ambiguous word form tags for same cohort, '" << cos[pos].form << "' vs '" << s.wftag << "'"<< std::endl;
				}
				cos[pos].readings.push_back({});
			}
			size_t level = cos[pos].readings.back().size();
			Reading n = { reindent(s.ana, level), "" };
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
			print_cohort(output, c);
		}
	}
}


void mwesplit(std::istream& is, UFILE *output)
{
	Cohort cohort = { "", {}, "" };
	size_t indentation = 0;
	unsigned int lno = 0;
	for (UString line; std::getline(is, line);) {
		++lno;
		UErrorCode status = U_ZERO_ERROR;
		uregex_setText(CG_LINE, line.c_str(), line.length(), &status);

		std::match_results<const char*> result;
		std::regex_match(line.c_str(), result, CG_LINE);
		if(!result.empty() && result[2].length() != 0) {
			split_and_print(output, cohort, lno);
			cohort = { result[2], {}, "" };
			indentation = 0;
		}
		else if(!result.empty() && result[3].length() != 0) {
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
			split_and_print(output, cohort, lno);
			cohort = { "", {} };
			indentation = 0;
			output << line << std::endl;
		}
	}

	split_and_print(output, cohort, lno);
}
}
