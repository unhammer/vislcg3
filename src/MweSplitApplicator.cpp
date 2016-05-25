/*
* Copyright (C) 2007-2016, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <mail@tinodidriksen.com>
*
* This file is part of VISL CG-3
*
* VISL CG-3 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* VISL CG-3 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with VISL CG-3.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "MweSplitApplicator.hpp"

namespace CG3 {

MweSplitApplicator::MweSplitApplicator(UFILE *ux_err)
	: GrammarApplicator(ux_err)
{
}


void MweSplitApplicator::runGrammarOnText(istream& input, UFILE *output) {
	GrammarApplicator::runGrammarOnText(input, output);
}


void MweSplitApplicator::printReading(const Reading *reading, UFILE *output, size_t sub) {
	if (reading->noprint) {
		return;
	}

	if (reading->deleted) {
		if (!trace) {
			return;
		}
		u_fputc(';', output);
	}

	for (size_t i = 0; i < sub; ++i) {
		u_fputc('\t', output);
	}

	if (reading->baseform) {
		u_fprintf(output, "%S", single_tags.find(reading->baseform)->second->tag.c_str());
	}

	uint32SortedVector unique;
	foreach (tter, reading->tags_list) {
		if ((!show_end_tags && *tter == endtag) || *tter == begintag) {
			continue;
		}
		if (*tter == reading->baseform || *tter == reading->parent->wordform->hash) {
			continue;
		}
		if (unique_tags) {
			if (unique.find(*tter) != unique.end()) {
				continue;
			}
			unique.insert(*tter);
		}
		const Tag *tag = single_tags[*tter];
		if (tag->type & T_DEPENDENCY && has_dep && !dep_original) {
			continue;
		}
		if (tag->type & T_RELATION && has_relations) {
			continue;
		}
		u_fprintf(output, " %S", tag->tag.c_str());
	}

	if (has_dep && !(reading->parent->type & CT_REMOVED)) {
		if (!reading->parent->dep_self) {
			reading->parent->dep_self = reading->parent->global_number;
		}
		const Cohort *pr = 0;
		pr = reading->parent;
		if (reading->parent->dep_parent != std::numeric_limits<uint32_t>::max()) {
			if (reading->parent->dep_parent == 0) {
				pr = reading->parent->parent->cohorts[0];
			}
			else if (reading->parent->parent->parent->cohort_map.find(reading->parent->dep_parent) != reading->parent->parent->parent->cohort_map.end()) {
				pr = reading->parent->parent->parent->cohort_map[reading->parent->dep_parent];
			}
		}

		const UChar local_utf_pattern[] = { ' ', '#', '%', 'u', L'\u2192', '%', 'u', 0 };
		const UChar local_latin_pattern[] = { ' ', '#', '%', 'u', '-', '>', '%', 'u', 0 };
		const UChar *pattern = local_latin_pattern;
		if (unicode_tags) {
			pattern = local_utf_pattern;
		}
		if (!dep_has_spanned) {
			u_fprintf_u(output, pattern,
			  reading->parent->local_number,
			  pr->local_number);
		}
		else {
			pattern = span_pattern_latin.c_str();
			if (unicode_tags) {
				pattern = span_pattern_utf.c_str();
			}
			if (reading->parent->dep_parent == std::numeric_limits<uint32_t>::max()) {
				u_fprintf_u(output, pattern,
				  reading->parent->parent->number,
				  reading->parent->local_number,
				  reading->parent->parent->number,
				  reading->parent->local_number);
			}
			else {
				u_fprintf_u(output, pattern,
				  reading->parent->parent->number,
				  reading->parent->local_number,
				  pr->parent->number,
				  pr->local_number);
			}
		}
	}

	if (reading->parent->type & CT_RELATED) {
		u_fprintf(output, " ID:%u", reading->parent->global_number);
		if (!reading->parent->relations.empty()) {
			foreach (miter, reading->parent->relations) {
				boost_foreach (uint32_t siter, miter->second) {
					u_fprintf(output, " R:%S:%u", grammar->single_tags.find(miter->first)->second->tag.c_str(), siter);
				}
			}
		}
	}

	if (trace) {
		foreach (iter_hb, reading->hit_by) {
			u_fputc(' ', output);
			printTrace(output, *iter_hb);
		}
	}

	u_fputc('\n', output);

	if (reading->next) {
		reading->next->deleted = reading->deleted;
		printReading(reading->next, output, sub + 1);
	}
}

const Tag* MweSplitApplicator::maybeWfTag(const Reading* r) {
	foreach (tter, r->tags_list) {
		if ((!show_end_tags && *tter == endtag) || *tter == begintag) {
			continue;
		}
		if (*tter == r->baseform || *tter == r->parent->wordform->hash) {
			continue;
		}
		const Tag *tag = single_tags[*tter];
		// If we are to split, there has to be at least one wordform on a head (not-sub) reading
		if (tag->type & T_WORDFORM) {
			return tag;
		}
	}
	return NULL;

}

std::vector<Cohort*> MweSplitApplicator::splitMwe(Cohort* cohort) {
	std::vector<Cohort*> cos;
	size_t n_wftags = 0;
	size_t n_goodreadings = 0;
	foreach (rter1, cohort->readings) {
		if(maybeWfTag(*rter1) != NULL) {
			++n_wftags;
		}
		++n_goodreadings;
	}

	if(n_wftags < n_goodreadings) {
		if(n_wftags > 0) {
			u_fprintf(ux_stderr, "WARNING: Line %u: Some but not all main-readings of \"<%s>\" had wordform-tags (not completely mwe-disambiguated?), not splitting.\n", numLines, cohort->wordform);
			// We also don't split if wordform-tags were only on sub-readings, but should we warn on such faulty input?
		}
		cos.push_back(cohort);
		return cos;
	}
	u_fprintf(ux_stderr, "splitting '%S'\n", cohort->wordform->tag.c_str());
	// TODO: split
	foreach(r, cohort->readings) {
		size_t pos = -1;
		Reading *prev = NULL;	// prev->next == sub || prev == NULL
		Reading *sub = (*r);
		while (sub) {
			const Tag* wfTag = maybeWfTag(sub);
			if(wfTag != NULL) {
				++pos;
				while(cos.size() < pos+1) {
					Cohort* c = alloc_cohort(cohort->parent);
					c->global_number = gWindow->cohort_counter++;
					c->local_number = cohort->local_number; // maybe look at what ADDCOHORT does
					UString blanks;
					blanks += ' '; blanks += '\n'; blanks += '\t'; blanks += '\r';
					size_t i = 1 + wfTag->tag.find_last_not_of(blanks);;
					c->wordform = addTag(wfTag->tag.substr(0, i));
					c->text = wfTag->tag.substr(i);
					u_fprintf(ux_stderr, "cos.push_back '%S' (postblank: '%S')\n", c->wordform->tag.c_str(), c->text.c_str());
					cos.push_back(c);
					if(cos[pos]->wordform != c->wordform) {
						u_fprintf(ux_stderr, "WARNING: Line %u: Ambiguous word form tags for same cohort, '%S' vs '%S'\n", cos[pos]->wordform->tag.c_str(), c->wordform->tag.c_str());
					}
				}
				if(prev != NULL) {
					// prev->next->deleted = true;
					prev->next = 0;
				}
			}
			// 		size_t level = cos[pos]->readings.back()->size();
			Reading *r = new Reading(*sub); // leak TODO
			// 		Reading n = { reindent(s.ana, level), "" };
			// TODO: here we want to make r a subreading of the correct level, so it has to be the "next" of the last r
			cos[pos]->appendReading(r);
			prev = sub;
			sub = sub->next;
		}
	}
	// The last word forms are the top readings:
	std::reverse(cos.begin(), cos.end());
	return cos;
}

void splitMwe2(bool trace, Cohort& cohort) {
	std::map<uint32_t, ReadingList> mlist;
	foreach (iter, cohort.readings) {
		Reading *r = *iter;
		uint32_t hp = r->hash; // instead of hash_plain, which doesn't include mapping tags
		if (trace) {
			foreach (iter_hb, r->hit_by) {
				hp = hash_value(*iter_hb, hp);
			}
		}
		Reading *sub = r->next;
		while (sub) {
			hp = hash_value(sub->hash, hp);
			if (trace) {
				foreach (iter_hb, sub->hit_by) {
					hp = hash_value(*iter_hb, hp);
				}
			}
			sub = sub->next;
		}
		mlist[hp].push_back(r);
	}

	if (mlist.size() == cohort.readings.size()) {
		return;
	}

	cohort.readings.clear();
	std::vector<Reading*> order;

	std::map<uint32_t, ReadingList>::iterator miter;
	for (miter = mlist.begin(); miter != mlist.end(); miter++) {
		ReadingList clist = miter->second;
		// no merging of mapping tags, so just take first reading of the group
		order.push_back(clist.front());

		clist.erase(clist.begin());
		foreach (cit, clist) {
			free_reading(*cit);
		}
	}

	std::sort(order.begin(), order.end(), CG3::Reading::cmp_number);
	cohort.readings.insert(cohort.readings.begin(), order.begin(), order.end());
}

void MweSplitApplicator::printCohort(Cohort *cohort, UFILE *output) {
	const UChar ws[] = { ' ', '\t', 0 };

	if (cohort->local_number == 0) {
		goto removed;
	}

	if (cohort->type & CT_REMOVED) {
		if (!trace || trace_no_removed) {
			goto removed;
		}
		u_fputc(';', output);
		u_fputc(' ', output);
	}
	u_fprintf(output, "%S", cohort->wordform->tag.c_str());
	if (cohort->wread) {
		foreach (tter, cohort->wread->tags_list) {
			if (*tter == cohort->wordform->hash) {
				continue;
			}
			const Tag *tag = single_tags[*tter];
			u_fprintf(output, " %S", tag->tag.c_str());
		}
	}
	u_fputc('\n', output);

	if (!split_mappings) {
		mergeMappings(*cohort);
	}

	foreach (rter1, cohort->readings) {
		printReading(*rter1, output);
	}
	if (trace && !trace_no_removed) {
		foreach (rter3, cohort->delayed) {
			printReading(*rter3, output);
		}
		foreach (rter2, cohort->deleted) {
			printReading(*rter2, output);
		}
	}

removed:
	if (!cohort->text.empty() && cohort->text.find_first_not_of(ws) != UString::npos) {
		u_fprintf(output, "%S", cohort->text.c_str());
		if (!ISNL(cohort->text[cohort->text.length() - 1])) {
			u_fputc('\n', output);
		}
	}

	foreach (iter, cohort->removed) {
		printCohort(*iter, output);
	}
}

void MweSplitApplicator::printSingleWindow(SingleWindow *window, UFILE *output) {
	boost_foreach (uint32_t var, window->variables_output) {
		Tag *key = single_tags[var];
		BOOST_AUTO(iter, window->variables_set.find(var));
		if (iter != window->variables_set.end()) {
			if (iter->second != grammar->tag_any) {
				Tag *value = single_tags[iter->second];
				u_fprintf(output, "%S%S=%S>\n", stringbits[S_CMD_SETVAR].getTerminatedBuffer(), key->tag.c_str(), value->tag.c_str());
			}
			else {
				u_fprintf(output, "%S%S>\n", stringbits[S_CMD_SETVAR].getTerminatedBuffer(), key->tag.c_str());
			}
		}
		else {
			u_fprintf(output, "%S%S>\n", stringbits[S_CMD_REMVAR].getTerminatedBuffer(), key->tag.c_str());
		}
	}

	if (!window->text.empty()) {
		u_fprintf(output, "%S", window->text.c_str());
		if (!ISNL(window->text[window->text.length() - 1])) {
			u_fputc('\n', output);
		}
	}

	uint32_t cs = (uint32_t)window->cohorts.size();
	for (uint32_t c = 0; c < cs; c++) {
		Cohort *cohort = window->cohorts[c];
		std::vector<Cohort*> cs = splitMwe(cohort);
		foreach(iter, cs) {
			u_fprintf(ux_stderr, "printCohort '%S'\n", (*iter)->wordform->tag.c_str());
			printCohort(*iter, output);
		}
	}
	u_fputc('\n', output);
	u_fflush(output);
}
}
