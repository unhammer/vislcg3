/*
 * Copyright (C) 2006, GrammarSoft Aps
 * and the VISL project at the University of Southern Denmark.
 * All Rights Reserved.
 *
 * The contents of this file are subject to the GrammarSoft Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.grammarsoft.com/GSPL or
 * http://visl.sdu.dk/GSPL.txt
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 */
#ifndef __GRAMMAR_H
#define __GRAMMAR_H

#include <unicode/ustring.h>
#include "Set.h"
#include "Rule.h"

namespace CG3 {

	class Grammar {
	public:
		unsigned int last_modified;
		UChar *name;
		unsigned int lines;
		unsigned int sections;
		stdext::hash_map<unsigned long, CompositeTag*> tags;
		stdext::hash_map<unsigned long, Set*> sets;
		stdext::hash_map<UChar*, unsigned long> delimiters;
		stdext::hash_map<UChar*, unsigned long> preferred_targets;
		stdext::hash_map<unsigned int, Rule*> rules;

		Grammar() {
			last_modified = 0;
			name = 0;
			lines = 0;
			sections = 0;
		}

		void addDelimiter(UChar *to) {
			UChar *delim = new UChar[u_strlen(to)+1];
			u_strcpy(delim, to);
			delimiters[delim] = hash_sdbm_uchar(delim);
		}
		void addPreferredTarget(UChar *to) {
			UChar *pf = new UChar[u_strlen(to)+1];
			u_strcpy(pf, to);
			preferred_targets[pf] = hash_sdbm_uchar(pf);
		}
		void addSet(Set *to) {
			if (sets[hash_sdbm_uchar(to->name)]) {
				std::wcerr << "Warning: Overwrote set " << to->name << std::endl;
				destroySet(sets[hash_sdbm_uchar(to->name)]);
			}
			sets[hash_sdbm_uchar(to->name)] = to;
		}
		Set *getSet(unsigned long which) {
			return sets[which] ? sets[which] : 0;
		}

		Set *allocateSet() {
			return new Set;
		}
		void destroySet(Set *set) {
			delete set;
		}

		void addCompositeTagToSet(Set *set, CompositeTag *tag) {
			if (tag && tag->tags.size()) {
				tag->rehash();
				tags[tag->getHash()] = tag;
				set->addCompositeTag(tag);
			} else {
				std::cerr << "Error: Attempted to add empty tag to grammar and set." << std::endl;
			}
		}
		CompositeTag *allocateCompositeTag() {
			return new CompositeTag;
		}
		void destroyCompositeTag(CompositeTag *tag) {
			delete tag;
		}

		void manipulateSet(unsigned long set_a, int op, unsigned long set_b, Set *result) {
			if (op <= S_IGNORE || op >= STRINGS_COUNT) {
				std::wcerr << "Error: Invalid set operation on line " << lines << std::endl;
				return;
			}
			if (!sets[set_a]) {
				std::wcerr << "Error: Invalid left operand for set operation on line " << lines << std::endl;
				return;
			}
			if (!sets[set_b]) {
				std::wcerr << "Error: Invalid right operand for set operation on line " << lines << std::endl;
				return;
			}
			if (!result) {
				std::wcerr << "Error: Invalid target for set operation on line " << lines << std::endl;
				return;
			}
			const UChar *tmpa = sets[set_a]->getName();
			const UChar *tmpb = sets[set_b]->getName();
			Set *stmpa = sets[set_a];
			Set *stmpb = sets[set_b];
			switch (op) {
				case S_MULTIPLY:
				case S_DENY:
				case S_NOT:
				case S_PLUS:
				case S_OR:
				{
					stdext::hash_map<unsigned long, CompositeTag*>::iterator iter;
					for (iter = sets[set_a]->tags.begin() ; iter != sets[set_a]->tags.end() ; iter++) {
						if (!result->tags[iter->first]) {
							if (true || iter->second) {
								addCompositeTagToSet(result, iter->second);
							}
						}
					}
					for (iter = sets[set_b]->tags.begin() ; iter != sets[set_b]->tags.end() ; iter++) {
						if (!result->tags[iter->first]) {
							if (true || iter->second) {
								addCompositeTagToSet(result, iter->second);
							}
						}
					}
					break;
				}
				case S_MINUS:
				{
					stdext::hash_map<unsigned long, CompositeTag*>::iterator iter;
					for (iter = sets[set_a]->tags.begin() ; iter != sets[set_a]->tags.end() ; iter++) {
						if (!result->tags[iter->first]) {
							addCompositeTagToSet(result, iter->second);
						}
					}
					for (iter = sets[set_b]->tags.begin() ; iter != sets[set_b]->tags.end() ; iter++) {
						if (result->tags[iter->first]) {
							result->removeCompositeTag(iter->first);
						}
					}
					break;
				}
				default:
					std::wcerr << "Error: Invalid set operation " << op << " between " << sets[set_a]->getName() << " and " << sets[set_b]->getName() << std::endl;
					break;
			}
		}
	};

}

#endif
