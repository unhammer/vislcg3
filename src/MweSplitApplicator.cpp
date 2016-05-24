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

	size_t n_wftags = 0;
	foreach (rter1, cohort->readings) {
		foreach (tter, (*rter1)->tags_list) {
			if ((!show_end_tags && *tter == endtag) || *tter == begintag) {
				continue;
			}
			if (*tter == (*rter1)->baseform || *tter == (*rter1)->parent->wordform->hash) {
				continue;
			}
			const Tag *tag = single_tags[*tter];
			u_fprintf(output, "tag: %S %u\n", tag->tag.c_str(), tag->type);
			// If we are to split, there has to be at least one wordform on a head (not-sub) reading
			if (tag->type & T_WORDFORM) {
				++n_wftags;
				u_fprintf(output, "WORDFORM: %S %u\n", tag->tag.c_str(), tag->type);
			}
		}
		u_fprintf(output, "\n");
		if ((*rter1)->next) {
			(*rter1)->next->deleted = (*rter1)->deleted;
			foreach (tter, (*rter1)->next->tags_list) {
				if ((!show_end_tags && *tter == endtag) || *tter == begintag) {
					continue;
				}
				if (*tter == (*rter1)->next->baseform || *tter == (*rter1)->next->parent->wordform->hash) {
					continue;
				}
				const Tag *tag = single_tags[*tter];
				u_fprintf(output, "tag: %S %u\n", tag->tag.c_str(), tag->type);
				// If we are to split, there has to be at least one wordform on a head (not-sub) reading
				if (tag->type & T_WORDFORM) {
					u_fprintf(output, "WORDFORM: %S %u\n", tag->tag.c_str(), tag->type);
				}
			}
		}
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
		printCohort(cohort, output);
	}
	u_fputc('\n', output);
	u_fflush(output);
}
}
