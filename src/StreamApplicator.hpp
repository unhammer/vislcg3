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
#ifndef a8f3031fe846e331_STREAMAPPLICATOR_HPP
#define a8f3031fe846e331_STREAMAPPLICATOR_HPP

#include <unicode/ustream.h>

#include "GrammarApplicator.hpp"
#include "Strings.hpp"
#include "Tag.hpp"
#include "Grammar.hpp"
#include "Window.hpp"
#include "SingleWindow.hpp"
#include "Reading.hpp"
#include "Cohort.hpp"

namespace CG3 {

class StreamApplicator : public virtual GrammarApplicator {
private:

public:
	StreamApplicator(UFILE *ux_err);
	void runGrammarOnText(std::stringstream& input, std::ostream& output);
	void runGrammarOnText(CG3::istream& input, std::ostream& output);
	void printSingleWindow(SingleWindow *window, std::ostream& output);
	void printReading(const Reading *reading, std::ostream& output, size_t sub = 1);
	void printCohort(Cohort *cohort, std::ostream& output);

};
}

#endif