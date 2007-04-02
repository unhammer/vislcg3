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
#ifndef __CONTEXTUALTEST_H
#define __CONTEXTUALTEST_H

#include "stdafx.h"
#include "Strings.h"

namespace CG3 {

	class ContextualTest {
	public:
		uint32_t line;
		uint32_t hash;
		bool careful;
		bool negated;
		bool negative;
		bool scanfirst;
		bool scanall;
		bool absolute;
		bool span_right;
		bool span_left;
		bool span_both;
		bool dep_parent;
		bool dep_sibling;
		bool dep_child;
		int32_t offset;

		uint32_t target;
		uint32_t barrier;
		uint32_t cbarrier;

		mutable uint32_t num_fail, num_match;
		mutable PACC_TimeStamp total_time;

		ContextualTest *linked;

		ContextualTest();
		~ContextualTest();

		void parsePosition(const UChar *pos, UFILE *ux_stderr);

		ContextualTest *allocateContextualTest();
		void destroyContextualTest(ContextualTest *to);
		
		uint32_t rehash();
		void reset();

		static bool cmp_quality(ContextualTest *a, ContextualTest *b);
	};

}

#endif
