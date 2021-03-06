/*
* Copyright (C) 2007-2017, GrammarSoft ApS
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

#pragma once
#ifndef c6d28b7452ec699b_GRAMMARAPPLICATORMATXIN_H
#define c6d28b7452ec699b_GRAMMARAPPLICATORMATXIN_H

#include "GrammarApplicator.hpp"

namespace CG3 {
class MatxinApplicator : public virtual GrammarApplicator {
public:
	MatxinApplicator(UFILE *ux_err);

	void runGrammarOnText(istream& input, UFILE *output);

	bool getNullFlush();
	bool wordform_case;
	bool print_word_forms;
	bool print_only_first;
	void setNullFlush(bool pNullFlush);

	void testPR(UFILE *output);

protected:
	struct Node {
		int self;
		UString lemma;
		UString form;
		UString pos;
		UString mi;
		UString si;
	};

	std::map<int, Node> nodes;
	std::map<int, std::vector<int> > deps;

	bool nullFlush;
	bool runningWithNullFlush;

	void printReading(Reading *reading, Node& n, UFILE *output);
	void printSingleWindow(SingleWindow *window, UFILE *output);

	void procNode(int& depth, std::map<int, Node>& nodes, std::map<int, std::vector<int> >& deps, int node, UFILE *output);


	void runGrammarOnTextWrapperNullFlush(istream& input, UFILE *output);

	UChar u_fgetc_wrapper(istream& input);
	UConverter *fgetc_converter;
	char fgetc_inputbuf[5];
	UChar fgetc_outputbuf[5];
	UErrorCode fgetc_error;
	void mergeMappings(Cohort& cohort);

private:
	void processReading(Reading *cReading, const UChar *reading_string);
	void processReading(Reading *cReading, const UString& reading_string);
};
}

#endif
