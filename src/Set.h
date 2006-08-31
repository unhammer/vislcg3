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
#ifndef __SET_H
#define __SET_H

#include <set>
#include <unicode/ustring.h>
#include "Grammar.h"
#include "CompositeTag.h"
#include "Strings.h"

namespace CG3 {

	class Set {
	public:
		UChar *name;
		uint32_t line;
		stdext::hash_map<UChar*, uint32_t> index_requires; // Simple common tags across the set
		stdext::hash_map<UChar*, uint32_t> index_certain;
		stdext::hash_map<UChar*, uint32_t> index_possible;
		stdext::hash_map<UChar*, uint32_t> index_impossible;
		stdext::hash_map<uint32_t, CompositeTag*> tags;

		Set() {
			name = 0;
			line = 0;
		}
		
		~Set() {
			if (name) {
				delete name;
			}
		}

		void setName(uint32_t to) {
			if (!to) {
				to = (uint32_t)rand();
			}
			name = new UChar[32];
			memset(name, 0, 32);
			u_sprintf(name, "_G_%u_%u_", line, to);
		}
		void setName(const UChar *to) {
			if (to) {
				name = new UChar[u_strlen(to)+1];
				u_strcpy(name, to);
			} else {
				setName((uint32_t)rand());
			}
		}
		const UChar *getName() {
			return name;
		}

		void setLine(uint32_t to) {
			line = to;
		}
		uint32_t getLine() {
			return line;
		}

		void addCompositeTag(CompositeTag *tag) {
			if (tag && tag->tags.size()) {
				tags[tag->rehash()] = tag;
			} else {
				u_fprintf(ux_stderr, "Error: Attempted to add empty tag to set!\n");
			}
		}
/*
		void removeCompositeTag(CompositeTag *tag) {
			tags[tag->getHash()] = 0;
			tags.erase(tag->getHash());
		}
		void removeCompositeTag(uint32_t tag) {
			tags[tag] = 0;
			tags.erase(tag);
		}
//*/
	};

}

#endif
