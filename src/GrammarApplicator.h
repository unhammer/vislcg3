/*
* Copyright (C) 2007, GrammarSoft ApS
* Developed by Tino Didriksen <tino@didriksen.cc>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <tino@didriksen.cc>
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

#ifndef __GRAMMARAPPLICATOR_H
#define __GRAMMARAPPLICATOR_H

#include "stdafx.h"
#include "Strings.h"
#include "Tag.h"
#include "Grammar.h"
#include "Window.h"
#include "SingleWindow.h"

namespace CG3 {
	class GrammarApplicator {
	public:
		bool always_span;
		bool apply_mappings;
		bool apply_corrections;
		bool no_before_sections;
		bool no_sections;
		bool no_after_sections;
		bool trace;
		bool trace_name_only;
		bool trace_no_removed;
		bool single_run;
		bool allow_magic_readings;

		bool dep_reenum;
		bool dep_delimit;
		bool dep_humanize;
		bool dep_original;
		bool dep_block_loops;

		uint32_t num_windows;
		uint32_t soft_limit;
		uint32_t hard_limit;
		uint32_t sections;
		uint32_t verbosity_level;

		GrammarApplicator(UFILE *ux_in, UFILE *ux_out, UFILE *ux_err);
		virtual ~GrammarApplicator();

		void enableStatistics();
		void disableStatistics();

		void setGrammar(const Grammar *res);

		virtual int runGrammarOnText(UFILE *input, UFILE *output);


	protected:

		void printReading(Reading *reading, UFILE *output);
		void printSingleWindow(SingleWindow *window, UFILE *output);

		UFILE *ux_stdin;
		UFILE *ux_stdout;
		UFILE *ux_stderr;

		uint32_t numLines;
		uint32_t numWindows;
		uint32_t numCohorts;
		uint32_t numReadings;

		static const uint32_t RV_NOTHING = 1;
		static const uint32_t RV_SOMETHING = 2;
		static const uint32_t RV_DELIMITED = 4;

		uint32_t skipped_rules;
		uint32_t cache_hits, cache_miss, match_single, match_comp, match_sub;
		uint32_t begintag, endtag;
		uint32_t last_mapping_tag;

		const Grammar *grammar;

		stdext::hash_map<uint32_t, uint32_t> variables;
		stdext::hash_map<uint32_t, uint32_t> metas;

		bool unif_mode;
		stdext::hash_map<uint32_t, uint32_t> unif_tags;

		stdext::hash_map<uint32_t, Tag*> single_tags;
		stdext::hash_map<uint32_t, uint32HashSet*> index_tags_regexp;

		stdext::hash_map<uint32_t, uint32HashSet*> index_reading_yes;
		stdext::hash_map<uint32_t, uint32HashSet*> index_reading_no;
		void resetIndexes();
	
		uint32_t addTag(const UChar *tag);

		int runGrammarOnWindow(Window *window);
		uint32_t runRulesOnWindow(SingleWindow *current, const int32_t start, const int32_t end);

		Cohort *runSingleTest(SingleWindow *sWindow, size_t i, const ContextualTest *test, bool *brk, bool *retval);
		Cohort *runContextualTest(SingleWindow *sWindow, const size_t position, const ContextualTest *test);

		bool doesTagMatchSet(const uint32_t tag, const uint32_t set);
		bool doesTagMatchReading(const Reading *reading, const uint32_t tag, bool bypass_index = false);
		bool doesSetMatchReading(Reading *reading, const uint32_t set, bool bypass_index = false);
		bool doesSetMatchCohortNormal(const Cohort *cohort, const uint32_t set);
		bool doesSetMatchCohortCareful(const Cohort *cohort, const uint32_t set);
		Cohort *doesSetMatchDependency(SingleWindow *sWindow, const Cohort *current, const ContextualTest *test);

		SingleWindow *initialiseSingleWindow(Recycler *r, Window *cWindow, SingleWindow *cSWindow);


		bool has_dep;
		uint32_t dep_highest_seen;
		Window *gWindow;
		bool statistics;

		inline bool __index_matches(const stdext::hash_map<uint32_t, uint32HashSet*> *me, const uint32_t value, const uint32_t set);
		void reflowReading(Reading *reading);
		void addTagToReading(Reading *reading, uint32_t tag);
		void delTagFromReading(Reading *reading, uint32_t tag);
		void reflowDependencyWindow();
		bool wouldParentChildLoop(Cohort *parent, Cohort *child);
		void attachParentChild(Cohort *parent, Cohort *child);
	};
}

#endif
