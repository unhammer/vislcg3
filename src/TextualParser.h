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
#ifndef __TEXTUALPARSER_H
#define __TEXTUALPARSER_H

#include "IGrammarParser.h"

namespace CG3 {
	class TextualParser : public IGrammarParser {
	public:
		TextualParser(UFILE *ux_in, UFILE *ux_out, UFILE *ux_err);
		~TextualParser();

		void setCompatible(bool compat);
		void setResult(CG3::Grammar *result);

		int parse_grammar_from_file(const char *filename, const char *locale, const char *codepage);

	private:
		UFILE *ux_stdin;
		UFILE *ux_stdout;
		UFILE *ux_stderr;
		bool option_vislcg_compat;
		bool in_section, in_before_sections, in_after_sections;
		const char *filename;
		const char *locale;
		const char *codepage;
		CG3::Grammar *result;

		int parseFromUChar(UChar *input);
		void addRuleToGrammar(Rule *rule);

		int parseTagList(Set *s, UChar **p, const bool isinline);
		int parseSetInline(Set *s, UChar **p);
		int parseContextualTestList(Rule *rule, std::list<ContextualTest*> *thelist, CG3::ContextualTest *parentTest, UChar **p);
		int parseContextualTests(Rule *rule, UChar **p);
		int parseContextualDependencyTests(Rule *rule, UChar **p);
		int parseRule(KEYWORDS key, UChar **p);
		int dieIfKeyword(UChar *s);

		inline bool ISSTRING(UChar *p, uint32_t c) {
			if (*(p-1) == '"' && *(p+c+1) == '"') {
				return true;
			}
			if (*(p-1) == '<' && *(p+c+1) == '>') {
				return true;
			}
			return false;
		}

		inline bool ISNL(const UChar c) {
			return (
			   c == 0x2028L // Unicode Line Seperator
			|| c == 0x2029L // Unicode Paragraph Seperator
			|| c == 0x0085L // EBCDIC NEL
			|| c == 0x000CL // Form Feed
			|| c == 0x000AL // ASCII \n
			);
		}

		inline bool ISESC(UChar *p) {
			uint32_t a=1;
			while(*(p-a) && *(p-a) == '\\') {
				a++;
			}
			return (a%2==0);
		}

		inline bool ISCHR(const UChar p, const UChar a, const UChar b) {
			return ((p) && ((p) == (a) || (p) == (b)));
		}

		inline uint32_t BACKTONL(UChar **p) {
			while (**p && !ISNL(**p)) {
				(*p)--;
			}
			(*p)++;
			return 1;
		}

		inline uint32_t SKIPLN(UChar **p) {
			while (**p && !ISNL(**p)) {
				(*p)++;
			}
			(*p)++;
			return 1;
		}

		inline uint32_t SKIPWS(UChar **p, const UChar a = 0, const UChar b = 0) {
			uint32_t s = 0;
			while (**p && **p != a && **p != b) {
				if (ISNL(**p)) {
					s++;
				}
				if (**p == '#' && !ISESC(*p)) {
					s += SKIPLN(p);
					(*p)--;
				}
				if (!u_isWhitespace(**p)) {
					break;
				}
				(*p)++;
			}
			return s;
		}

		inline uint32_t SKIPTOWS(UChar **p, const UChar a = 0, const bool allowhash = false) {
			uint32_t s = 0;
			while (**p && !u_isWhitespace(**p) && **p != a) {
				if (!allowhash && **p == '#' && !ISESC(*p)) {
					s += SKIPLN(p);
					(*p)--;
				}
				if (**p == ';' && !ISESC(*p)) {
					break;
				}
				if (**p == a && !ISESC(*p)) {
					break;
				}
				(*p)++;
			}
			return s;
		}
	};
}

#endif
