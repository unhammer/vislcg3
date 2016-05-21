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
#include "Strings.hpp"
#include "Tag.hpp"
#include "Grammar.hpp"
#include "Window.hpp"
#include "SingleWindow.hpp"
#include "Reading.hpp"

namespace CG3 {

MweSplitApplicator::MweSplitApplicator(UFILE *ux_err)
  : GrammarApplicator(ux_err)
{
}


void MweSplitApplicator::runGrammarOnText(istream& input, UFILE *output) {
	GrammarApplicator::runGrammarOnText(input, output);
}


void MweSplitApplicator::printReading(const Reading *reading, UFILE *output) {
	if (reading->noprint) {
		return;
	}
	if (reading->deleted) {
		return;
	}
	u_fputc('\t', output);
	if (reading->baseform) {
		u_fprintf(output, "[%.*S]", single_tags.find(reading->baseform)->second->tag.size() - 2, single_tags.find(reading->baseform)->second->tag.c_str() + 1);
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
			if (reading->parent->dep_parent == std::numeric_limits<uint32_t>::max()) {
				u_fprintf_u(output, pattern,
				  reading->parent->dep_self,
				  reading->parent->dep_self);
			}
			else {
				u_fprintf_u(output, pattern,
				  reading->parent->dep_self,
				  reading->parent->dep_parent);
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

	if (reading->next && !did_warn_subreadings) {
		u_fprintf(ux_stderr, "Warning: MweSplit CG format cannot output sub-readings! You are losing information!\n");
		u_fflush(ux_stderr);
		did_warn_subreadings = true;
	}
}

void MweSplitApplicator::printCohort(Cohort *cohort, UFILE *output) {
	const UChar ws[] = { ' ', '\t', 0 };

	if (cohort->local_number == 0) {
		goto removed;
	}
	if (cohort->type & CT_REMOVED) {
		goto removed;
	}

	u_fprintf(output, "%.*S", cohort->wordform->tag.size() - 4, cohort->wordform->tag.c_str() + 2);
	if (cohort->wread && !did_warn_statictags) {
		u_fprintf(ux_stderr, "Warning: MweSplit CG format cannot output static tags! You are losing information!\n");
		u_fflush(ux_stderr);
		did_warn_statictags = true;
	}

	if (!split_mappings) {
		mergeMappings(*cohort);
	}

	if (cohort->readings.empty()) {
		u_fputc('\t', output);
	}
	boost_foreach (Reading *rter, cohort->readings) {
		printReading(rter, output);
	}

removed:
	u_fputc('\n', output);
	if (!cohort->text.empty() && cohort->text.find_first_not_of(ws) != UString::npos) {
		u_fprintf(output, "%S", cohort->text.c_str());
		if (!ISNL(cohort->text[cohort->text.length() - 1])) {
			u_fputc('\n', output);
		}
	}
}

void MweSplitApplicator::printSingleWindow(SingleWindow *window, UFILE *output) {
	if (!window->text.empty()) {
		u_fprintf(output, "%S", window->text.c_str());
		if (!ISNL(window->text[window->text.length() - 1])) {
			u_fputc('\n', output);
		}
	}

	uint32_t cs = (uint32_t)window->cohorts.size();
	for (uint32_t c = 0; c < cs; c++) {
		Cohort *cohort = window->cohorts[c];
		printCohort(cohort, output);
	}
	u_fputc('\n', output);
	u_fflush(output);
}
}
