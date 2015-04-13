/*
* Copyright (C) 2007-2015, GrammarSoft ApS
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

#include "GrammarWriter.hpp"
#include "Strings.hpp"
#include "Grammar.hpp"
#include "ContextualTest.hpp"

namespace CG3 {

GrammarWriter::GrammarWriter(Grammar& res, UFILE *ux_err) {
	statistics = false;
	ux_stderr = ux_err;
	grammar = &res;
}

GrammarWriter::GrammarWriter(Grammar& res, UFILE *ux_err, UStringMap* relabels, UStringMap* relabels_simple) {
	statistics = false;
	ux_stderr = ux_err;
	grammar = &res;
	relabel_as_set = relabels;
	relabel_as_list = relabels_simple;

	set_id_map_t* ids = new set_id_map_t;
	uint32_t i = 0;
	boost_foreach (CG3::UStringMap::value_type kv, *relabel_as_set) {
		ids->emplace(kv.first, i);
		i++;
	}
	relabel_set_ids = ids;
}

GrammarWriter::~GrammarWriter() {
	grammar = 0;
	delete relabel_set_ids;
	relabel_set_ids = 0;
}

void GrammarWriter::printList(UFILE *output, const Set& curset) {
	if(relabel_as_set) {
		return printListRelabelled(output, curset);
	}

	u_fprintf(output, "LIST %S = ", curset.name.c_str());
	std::set<TagVector> tagsets[] = { trie_getTagsOrdered(curset.trie), trie_getTagsOrdered(curset.trie_special) };
	boost_foreach (const std::set<TagVector>& tvs, tagsets) {
		boost_foreach (const TagVector& tags, tvs) {
			if (tags.size() > 1) {
				u_fprintf(output, "(");
			}
			boost_foreach (const Tag* tag, tags) {
				printTag(output, *tag);
				u_fprintf(output, " ");
			}
			if (tags.size() > 1) {
				u_fprintf(output, ")");
			}
		}
	}
	u_fprintf(output, " ;\n");

}

bool GrammarWriter::needsSetOps(std::set<TagVector> tagsets[]) {
	// TODO: why doesn't boost_foreach work here? I get confusing make errors:
	// boost_foreach (const std::set<TagVector>& tvs, tagsets) {
	// }
	for (int i=0; i<2; i++) {
		const std::set<TagVector>& tvs = tagsets[i];
		boost_foreach (const TagVector& tags, tvs) {
		 	boost_foreach (const Tag* tag, tags) {
				UString tagName = tag->toUString(true);
				bool needs_relabelling = relabel_set_ids->find(tagName) != relabel_set_ids->end();
				bool needs_set_ops = relabel_as_list->find(tagName) == relabel_as_list->end();
				if(needs_relabelling && needs_set_ops) {
					return true;
				}
			}
		}
	}
	return false;
}

void GrammarWriter::printListRelabelled(UFILE *output, const Set& curset) {
	bool used_to_unify = unified_sets.find(curset.name) != unified_sets.end();
	std::set<TagVector> tagsets[] = { trie_getTagsOrdered(curset.trie), trie_getTagsOrdered(curset.trie_special) };
	bool needs_set_ops = needsSetOps(tagsets);
	if(used_to_unify && needs_set_ops) {
		u_fprintf(ux_stderr, "Warning: LIST's used in unification can't be relabelled using SET operations!\n");
	}
	bool treat_as_list = used_to_unify || !needs_set_ops ;
	if(treat_as_list) {
		u_fprintf(output, "LIST %S = ", curset.name.c_str());
		boost_foreach (const std::set<TagVector>& tvs, tagsets) {
			boost_foreach (const TagVector& tags, tvs) {
				// Output the parentheses unconditionally; otherwise we need to first go through the tags and check if any need relabelling into >1 tag
				u_fprintf(output, "(");
				boost_foreach (const Tag* tag, tags) {
					UString tagName = tag->toUString(true);
					bool relabel = relabel_as_list->find(tagName) != relabel_as_list->end();
					if(relabel) {
						u_fprintf(output, "%S ", relabel_as_list->at(tagName).c_str());
					}
					else {
						printTag(output, *tag);
						u_fprintf(output, " ");
					}
				}
				u_fprintf(output, ") ");
			}
		}
	}
	else {
		u_fprintf(output, "SET %S = ", curset.name.c_str());
		bool first = true;
		boost_foreach (const std::set<TagVector>& tvs, tagsets) {
			if (tvs.size() == 0) {
				continue;
			}
			boost_foreach (const TagVector& tags, tvs) {
				if (first) {
					first = false;
				}
				else {
					u_fprintf(output, " OR ");
				}
				bool need_plus = false;
				std::set<const Tag*> unaltered_tags;
				boost_foreach (const Tag* tag, tags) {
					UString tagName = tag->toUString(true);
					bool relabel = relabel_set_ids->find(tagName) != relabel_set_ids->end();
					if(relabel) {
						if(need_plus) {
							u_fprintf(output, "+ ");
						}
						u_fprintf(output, "CG3_RELABEL_%d ", relabel_set_ids->at(tagName));
						need_plus = true;
					}
					else {
						unaltered_tags.insert(tag);
					}
				}
				if(unaltered_tags.size()>0) {
					if(need_plus) {
						u_fprintf(output, "+ ");
					}
					u_fprintf(output, "(");
					boost_foreach (const Tag* tag, unaltered_tags) {
						printTag(output, *tag);
						u_fprintf(output, " ");
					}
					u_fprintf(output, ")");
				}
			}
		}
	}
	u_fprintf(output, " ;\n");

}

void GrammarWriter::printSet(UFILE *output, const Set& curset) {
	if (used_sets.find(curset.number) != used_sets.end()) {
		return;
	}

	if (curset.sets.empty()) {
		if (statistics) {
			if (ceil(curset.total_time) == floor(curset.total_time)) {
				u_fprintf(output, "#List Matched: %u ; NoMatch: %u ; TotalTime: %.0f\n", curset.num_match, curset.num_fail, curset.total_time);
			}
			else {
				u_fprintf(output, "#List Matched: %u ; NoMatch: %u ; TotalTime: %f\n", curset.num_match, curset.num_fail, curset.total_time);
			}
		}
		used_sets.insert(curset.number);
		printList(output, curset);
	}
	else {
		used_sets.insert(curset.number);
		for (uint32_t i=0;i<curset.sets.size();i++) {
			printSet(output, *(grammar->sets_list[curset.sets[i]]));
		}
		if (statistics) {
			if (ceil(curset.total_time) == floor(curset.total_time)) {
				u_fprintf(output, "#Set Matched: %u ; NoMatch: %u ; TotalTime: %.0f\n", curset.num_match, curset.num_fail, curset.total_time);
			}
			else {
				u_fprintf(output, "#Set Matched: %u ; NoMatch: %u ; TotalTime: %f\n", curset.num_match, curset.num_fail, curset.total_time);
			}
		}
		const UChar *n = curset.name.c_str();
		if ((n[0] == '$' && n[1] == '$') || (n[0] == '&' && n[1] == '&')) {
			u_fprintf(output, "# ");
		}
		u_fprintf(output, "SET %S = ", n);
		u_fprintf(output, "%S ", grammar->sets_list[curset.sets[0]]->name.c_str());
		for (uint32_t i=0;i<curset.sets.size()-1;i++) {
			u_fprintf(output, "%S %S ", stringbits[curset.set_ops[i]].getTerminatedBuffer(), grammar->sets_list[curset.sets[i+1]]->name.c_str());
		}
		u_fprintf(output, " ;\n\n");
	}
}

int GrammarWriter::writeGrammar(UFILE *output) {
	if (!output) {
		u_fprintf(ux_stderr, "Error: Output is null - cannot write to nothing!\n");
		CG3Quit(1);
	}
	if (!grammar) {
		u_fprintf(ux_stderr, "Error: No grammar provided - cannot continue!\n");
		CG3Quit(1);
	}
	if (grammar->is_binary) {
		u_fprintf(ux_stderr, "Error: Grammar is binary and cannot be output in textual form!\n");
		CG3Quit(1);
	}

	if (statistics) {
		if (ceil(grammar->total_time) == floor(grammar->total_time)) {
			u_fprintf(output, "# Total ticks spent applying grammar: %.0f\n", grammar->total_time);
		}
		else {
			u_fprintf(output, "# Total ticks spent applying grammar: %f\n", grammar->total_time);
		}
	}
	u_fprintf(output, "# DELIMITERS and SOFT-DELIMITERS do not exist. Instead, look for the sets _S_DELIMITERS_ and _S_SOFT_DELIMITERS_.\n");

	u_fprintf(output, "MAPPING-PREFIX = %C ;\n", grammar->mapping_prefix);

	if (grammar->sub_readings_ltr) {
		u_fprintf(output, "SUBREADINGS = LTR ;\n");
	}
	else {
		u_fprintf(output, "SUBREADINGS = RTL ;\n");
	}

	if (!grammar->static_sets.empty()) {
		u_fprintf(output, "STATIC-SETS =");
		boost_foreach (const UString& str, grammar->static_sets) {
			u_fprintf(output, " %S", str.c_str());
		}
		u_fprintf(output, " ;\n");
	}

	if (!grammar->preferred_targets.empty()) {
		u_fprintf(output, "PREFERRED-TARGETS = ");
		uint32Vector::const_iterator iter;
		for (iter = grammar->preferred_targets.begin() ; iter != grammar->preferred_targets.end() ; iter++ ) {
			printTag(output, *(grammar->single_tags.find(*iter)->second));
			u_fprintf(output, " ");
		}
		u_fprintf(output, " ;\n");
	}

	u_fprintf(output, "\n");

	used_sets.clear();
	if(relabel_as_set) {
		printRelabelSets(output);
	}
	boost_foreach (Set *s, grammar->sets_list) {
		if (s->name[0] == '_' && s->name[1] == 'G' && s->name[2] == '_') {
			s->name.insert(s->name.begin(), '3');
			s->name.insert(s->name.begin(), 'G');
			s->name.insert(s->name.begin(), 'C');
		}
		if (s->name[0] == '$' && s->name[1] == '$') {
			unified_sets.insert(s->name.substr(2));
		}

	}
	boost_foreach (Set *s, grammar->sets_list) {
		if(s->number == 0) {
			// dummy set
			u_fprintf(output, "#");
		}
		printSet(output, *s);
	}
	u_fprintf(output, "\n");

	/*
	for (BOOST_AUTO(cntx, grammar->templates.begin()); cntx != grammar->templates.end(); ++cntx) {
		u_fprintf(output, "TEMPLATE %u = ", cntx->second->hash);
		printContextualTest(output, *cntx->second);
		u_fprintf(output, " ;\n");
	}
	u_fprintf(output, "\n");
	//*/

	bool found = false;
	const_foreach (RuleVector, grammar->rule_by_number, rule_iter, rule_iter_end) {
		const Rule& r = **rule_iter;
		if (r.section == -1) {
			if (!found) {
				u_fprintf(output, "\nBEFORE-SECTIONS\n");
				found = true;
			}
			printRule(output, r);
			u_fprintf(output, " ;\n");
		}
	}
	const_foreach (uint32Vector, grammar->sections, isec, isec_end) {
		found = false;
		const_foreach (RuleVector, grammar->rule_by_number, rule_iter, rule_iter_end) {
			const Rule& r = **rule_iter;
			if (r.section == (int32_t)*isec) {
				if (!found) {
					u_fprintf(output, "\nSECTION\n");
					found = true;
				}
				printRule(output, r);
				u_fprintf(output, " ;\n");
			}
		}
	}
	found = false;
	const_foreach (RuleVector, grammar->rule_by_number, rule_iter, rule_iter_end) {
		const Rule& r = **rule_iter;
		if (r.section == -2) {
			if (!found) {
				u_fprintf(output, "\nAFTER-SECTIONS\n");
				found = true;
			}
			printRule(output, r);
			u_fprintf(output, " ;\n");
		}
	}
	found = false;
	const_foreach (RuleVector, grammar->rule_by_number, rule_iter, rule_iter_end) {
		const Rule& r = **rule_iter;
		if (r.section == -3) {
			if (!found) {
				u_fprintf(output, "\nNULL-SECTION\n");
				found = true;
			}
			printRule(output, r);
			u_fprintf(output, " ;\n");
		}
	}

	return 0;
}

void GrammarWriter::printRelabelSets(UFILE *output) {
	boost_foreach (CG3::UStringMap::value_type kv, *relabel_as_set) {
		u_fprintf(output, "SET CG3_RELABEL_%d = %S ;\n", relabel_set_ids->at(kv.first), kv.second.c_str());
	}
}


// ToDo: Make printRule do the right thing for MOVE_ and ADDCOHORT_ BEFORE|AFTER
void GrammarWriter::printRule(UFILE *to, const Rule& rule) {
	if (statistics) {
		if (ceil(rule.total_time) == floor(rule.total_time)) {
			u_fprintf(to, "\n#Rule Matched: %u ; NoMatch: %u ; TotalTime: %.0f\n", rule.num_match, rule.num_fail, rule.total_time);
		}
		else {
			u_fprintf(to, "\n#Rule Matched: %u ; NoMatch: %u ; TotalTime: %f\n", rule.num_match, rule.num_fail, rule.total_time);
		}
	}
	if (rule.wordform) {
		printTag(to, *rule.wordform);
		u_fprintf(to, " ");
	}

	u_fprintf(to, "%S", keywords[rule.type].getTerminatedBuffer());

	if (rule.name && !(rule.name[0] == '_' && rule.name[1] == 'R' && rule.name[2] == '_')) {
		u_fprintf(to, ":%S", rule.name);
	}
	u_fprintf(to, " ");

	for (uint32_t i=0 ; i<FLAGS_COUNT ; i++) {
		if (rule.flags & (1 << i)) {
			u_fprintf(to, "%S ", flags[i].getTerminatedBuffer());
		}
	}

	if (rule.sublist) {
		u_fprintf(to, "%S ", rule.sublist->name.c_str());
	}

	if (rule.maplist) {
		u_fprintf(to, "%S ", rule.maplist->name.c_str());
	}

	if (rule.target) {
		u_fprintf(to, "%S ", grammar->sets_list[rule.target]->name.c_str());
	}

	const_foreach (ContextList, rule.tests, it, it_end) {
		u_fprintf(to, "(");
		printContextualTest(to, **it);
		u_fprintf(to, ") ");
	}

	if (rule.dep_target) {
		u_fprintf(to, "TO (");
		printContextualTest(to, *(rule.dep_target));
		u_fprintf(to, ") ");
		const_foreach (ContextList, rule.dep_tests, it, it_end) {
			u_fprintf(to, "(");
			printContextualTest(to, **it);
			u_fprintf(to, ") ");
		}
	}
}

void GrammarWriter::printContextualTest(UFILE *to, const ContextualTest& test) {
	if (statistics) {
		if (ceil(test.total_time) == floor(test.total_time)) {
			u_fprintf(to, "\n#Test Matched: %u ; NoMatch: %u ; TotalTime: %.0f\n", test.num_match, test.num_fail, test.total_time);
		}
		else {
			u_fprintf(to, "\n#Test Matched: %u ; NoMatch: %u ; TotalTime: %f\n", test.num_match, test.num_fail, test.total_time);
		}
	}
	if (test.tmpl) {
		u_fprintf(to, "T:%u ", test.tmpl->hash);
	}
	else if (!test.ors.empty()) {
		for (BOOST_AUTO(iter, test.ors.begin()) ; iter != test.ors.end() ; ) {
			u_fprintf(to, "(");
			printContextualTest(to, **iter);
			u_fprintf(to, ")");
			iter++;
			if (iter != test.ors.end()) {
				u_fprintf(to, " OR ");
			}
			else {
				u_fprintf(to, " ");
			}
		}
	}
	else {
		if (test.pos & POS_NEGATE) {
			u_fprintf(to, "NEGATE ");
		}
		if (test.pos & POS_ALL) {
			u_fprintf(to, "ALL ");
		}
		if (test.pos & POS_NONE) {
			u_fprintf(to, "NONE ");
		}
		if (test.pos & POS_NOT) {
			u_fprintf(to, "NOT ");
		}
		if (test.pos & POS_ABSOLUTE) {
			u_fprintf(to, "@");
		}
		if (test.pos & POS_SCANALL) {
			u_fprintf(to, "**");
		}
		else if (test.pos & POS_SCANFIRST) {
			u_fprintf(to, "*");
		}

		if (test.pos & POS_DEP_CHILD) {
			u_fprintf(to, "c");
		}
		if (test.pos & POS_DEP_PARENT) {
			u_fprintf(to, "p");
		}
		if (test.pos & POS_DEP_SIBLING) {
			u_fprintf(to, "s");
		}

		if (test.pos & POS_UNKNOWN) {
			u_fprintf(to, "?");
		}
		else {
			u_fprintf(to, "%d", test.offset);
		}

		if (test.pos & POS_CAREFUL) {
			u_fprintf(to, "C");
		}
		if (test.pos & POS_SPAN_BOTH) {
			u_fprintf(to, "W");
		}
		if (test.pos & POS_SPAN_LEFT) {
			u_fprintf(to, "<");
		}
		if (test.pos & POS_SPAN_RIGHT) {
			u_fprintf(to, ">");
		}
		if (test.pos & POS_PASS_ORIGIN) {
			u_fprintf(to, "o");
		}
		if (test.pos & POS_NO_PASS_ORIGIN) {
			u_fprintf(to, "O");
		}
		if (test.pos & POS_LEFT_PAR) {
			u_fprintf(to, "L");
		}
		if (test.pos & POS_RIGHT_PAR) {
			u_fprintf(to, "R");
		}
		if (test.pos & POS_MARK_SET) {
			u_fprintf(to, "X");
		}
		if (test.pos & POS_MARK_JUMP) {
			u_fprintf(to, "x");
		}
		if (test.pos & POS_LOOK_DELETED) {
			u_fprintf(to, "D");
		}
		if (test.pos & POS_LOOK_DELAYED) {
			u_fprintf(to, "d");
		}
		if (test.pos & POS_RELATION) {
			u_fprintf(to, "r:%S", grammar->single_tags.find(test.relation)->second->tag.c_str());
		}

		u_fprintf(to, " ");

		if (test.target) {
			u_fprintf(to, "%S ", grammar->sets_list[test.target]->name.c_str());
		}
		if (test.cbarrier) {
			u_fprintf(to, "CBARRIER %S ", grammar->sets_list[test.cbarrier]->name.c_str());
		}
		if (test.barrier) {
			u_fprintf(to, "BARRIER %S ", grammar->sets_list[test.barrier]->name.c_str());
		}
	}

	if (test.linked) {
		u_fprintf(to, "LINK ");
		printContextualTest(to, *(test.linked));
	}
}

void GrammarWriter::printTag(UFILE *to, const Tag& tag) {
	UString str = tag.toUString(true);
	u_file_write(str.c_str(), str.length(), to);
}

}
