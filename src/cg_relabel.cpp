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

#include "stdafx.hpp"
#include "Grammar.hpp"
#include "TextualParser.hpp"
#include "GrammarWriter.hpp"
#include "BinaryGrammar.hpp"
#include "GrammarApplicator.hpp"

#ifndef _WIN32
#include <libgen.h>
#endif

#include "version.hpp"

using CG3::CG3Quit;

void endProgram(char *name) {
	if (name != NULL) {
		fprintf(stdout, "VISL CG-3 Relabeller version %u.%u.%u.%u\n",
			CG3_VERSION_MAJOR, CG3_VERSION_MINOR, CG3_VERSION_PATCH, CG3_REVISION);
		std::cout << basename(name) << ": relabel a textual grammar using a relabelling file" << std::endl;
		std::cout << "USAGE: " << basename(name) << " input_grammar_file relabel_rule_file output_grammar_file" << std::endl;
	}
	exit(EXIT_FAILURE);
}

CG3::UStringMap* parse_relabel_file(const char *filename, const char *locale, const char *codepage, UFILE * ux_stderr) {
	const char * filebase = basename(const_cast<char*>(filename));

	struct stat _stat;
	int error = stat(filename, &_stat);

	if (error != 0) {
		u_fprintf(ux_stderr, "%s: Error: Cannot stat %s due to error %d - bailing out!\n", filebase, filename, error);
		CG3Quit(1);
	}

	UFILE *grammar = u_fopen(filename, "rb", locale, codepage);
	if (!grammar) {
		u_fprintf(ux_stderr, "%s: Error: Error opening %s for reading!\n", filebase, filename);
		CG3Quit(1);
	}
	UChar32 bom = u_fgetcx(grammar);
	if (bom != 0xfeff && bom != static_cast<UChar32>(0xffffffff)) {
		u_fungetc(bom, grammar);
	}

	CG3::UStringMap* relabel_rules = new CG3::UStringMap;

	// TODO: actually parse the file
	CG3::UString
		from(UNICODE_STRING_SIMPLE("N").getTerminatedBuffer()),
		to(UNICODE_STRING_SIMPLE("(n) OR (np)").getTerminatedBuffer()),
		from2(UNICODE_STRING_SIMPLE("Prop").getTerminatedBuffer()),
		to2(UNICODE_STRING_SIMPLE("(np) - (n)").getTerminatedBuffer());
	relabel_rules->emplace(from, to);
	relabel_rules->emplace(from2, to2);

	return relabel_rules;
}

int main(int argc, char *argv[]) {
	UFILE *ux_stderr = 0;
	UErrorCode status = U_ZERO_ERROR;

	if (argc != 4) {
		endProgram(argv[0]);
	}

	/* Initialize ICU */
	u_init(&status);
	if (U_FAILURE(status) && status != U_FILE_ACCESS_ERROR) {
		std::cerr << "Error: Cannot initialize ICU. Status = " << u_errorName(status) << std::endl;
		CG3Quit(1);
	}
	status = U_ZERO_ERROR;

	ucnv_setDefaultName("UTF-8");
	const char *codepage_default = ucnv_getDefaultName();
	uloc_setDefault("en_US_POSIX", &status);
	const char *locale_default = uloc_getDefault();

	ux_stderr = u_finit(stderr, locale_default, codepage_default);

	CG3::Grammar grammar;

	CG3::IGrammarParser *parser = 0;
	FILE *input = fopen(argv[1], "rb");

	if (!input) {
		std::cerr << "Error: Error opening " << argv[1] << " for reading!" << std::endl;
		CG3Quit(1);
	}
	if (fread(&CG3::cbuffers[0][0], 1, 4, input) != 4) {
		std::cerr << "Error: Error reading first 4 bytes from grammar!" << std::endl;
		CG3Quit(1);
	}
	fclose(input);

	if (CG3::cbuffers[0][0] == 'C' && CG3::cbuffers[0][1] == 'G' && CG3::cbuffers[0][2] == '3' && CG3::cbuffers[0][3] == 'B') {
		std::cerr << "Binary grammar detected. Cannot relabel binary grammars." << std::endl;
		CG3Quit(1);
	}
	else {
		parser = new CG3::TextualParser(grammar, ux_stderr);
	}

	grammar.ux_stderr = ux_stderr;

	if (parser->parse_grammar_from_file(argv[1], locale_default, codepage_default)) {
		std::cerr << "Error: Grammar could not be parsed - exiting!" << std::endl;
		CG3Quit(1);
	}

	grammar.reindex();

	std::cerr << "Sections: " << grammar.sections.size() << ", Rules: " << grammar.rule_by_number.size();
	std::cerr << ", Sets: " << grammar.sets_list.size() << ", Tags: " << grammar.single_tags.size() << std::endl;

	if (grammar.rules_any) {
		std::cerr << grammar.rules_any->size() << " rules cannot be skipped by index." << std::endl;
	}

	if (grammar.has_dep) {
		std::cerr << "Grammar has dependency rules." << std::endl;
	}

std::cerr<<"parsing relabel file:"<<std::endl;
	CG3::UStringMap* relabel_rules = parse_relabel_file(argv[2], locale_default, codepage_default, ux_stderr);

boost_foreach (CG3::UStringMap::value_type pair, *relabel_rules) {
	u_fprintf(ux_stderr, "%S --> %S\n", pair.first.c_str(), pair.second.c_str());
}

std::cerr<<"writing new grammar file:"<<std::endl;

	UFILE *gout = u_fopen(argv[3], "w", locale_default, codepage_default);

	if (gout) {
		CG3::GrammarWriter writer(grammar, ux_stderr, relabel_rules);
		writer.writeGrammar(gout);
	}
	else {
		std::cerr << "Could not write grammar to " << argv[3] << std::endl;
	}

	u_fclose(ux_stderr);

	u_cleanup();

	return status;
}


