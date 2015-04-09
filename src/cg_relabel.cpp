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


int parseFromUChar(CG3::UStringMap* relabel_rules, UChar *input, const char *fname, UFILE * ux_stderr) {
	if (!input || !input[0]) {
		u_fprintf(ux_stderr, "%s: Error: Input is empty - cannot continue!\n", fname);
		CG3Quit(1);
	}

	UChar *p = input;
	int lines = 1;
	const char * filebase = basename(const_cast<char*>(fname));

	lines += CG3::SKIPWS(p);
	while (*p) {
		// the "from" tag:
		UChar *n = p;
		lines += CG3::SKIPTOWS(n, 0, true);
		ptrdiff_t c = n - p;
		u_strncpy(&CG3::gbuffers[0][0], p, c);
		CG3::gbuffers[0][c] = 0;
		CG3::UString from = &CG3::gbuffers[0][0];
		p = n;
		// TO keyword:
		lines += CG3::SKIPWS(p);
		if (!(CG3::ISCHR(*p,'T','t') && CG3::ISCHR(*(p+1),'O','o') && CG3::ISSPACE(*(p+2)))) {
			u_fprintf(ux_stderr, "%s: Error: missing TO keyword on line %d!\n", filebase, lines);
			CG3Quit(1);
		}
		p += 2;
		// the "to" set:
		n = p;
		lines += CG3::SKIPLN(n);
		UChar *e = n;
		while (CG3::ISSPACE(*(e-1))) {
			--e;
		}
		c = e - p;
		u_strncpy(&CG3::gbuffers[0][0], p, c);
		CG3::gbuffers[0][c] = 0;
		CG3::UString to = &CG3::gbuffers[0][0];
		p = n;
		relabel_rules->emplace(from, to);
		lines += CG3::SKIPWS(p);
	}

	return 0;
}

int parse_relabel_file(CG3::UStringMap* relabel_rules, const char *filename, const char *locale, const char *codepage, UFILE * ux_stderr) {
	const char * filebase = basename(const_cast<char*>(filename));

	struct stat _stat;
	int error = stat(filename, &_stat);
	size_t grammar_size = 0;

	if (error != 0) {
		u_fprintf(ux_stderr, "%s: Error: Cannot stat %s due to error %d - bailing out!\n", filebase, filename, error);
		CG3Quit(1);
	}
	else {
		grammar_size = static_cast<size_t>(_stat.st_size);
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

	// It reads into the buffer at offset 4 because certain functions may look back, so we need some nulls in front.
	std::vector<UChar> data(grammar_size*2, 0);
	uint32_t read = u_file_read(&data[4], grammar_size*2, grammar);
	u_fclose(grammar);
	if (read >= grammar_size*2-1) {
		u_fprintf(ux_stderr, "%s: Error: Converting from underlying codepage to UTF-16 exceeded factor 2 buffer.\n", filebase);
		CG3Quit(1);
	}
	data.resize(read+4+1);


	error = parseFromUChar(relabel_rules, &data[4], filename, ux_stderr);
	if (error) {
		return error;
	}

	return 0;
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

	delete parser;
	parser = 0;

	std::cerr << "Sections: " << grammar.sections.size() << ", Rules: " << grammar.rule_by_number.size();
	std::cerr << ", Sets: " << grammar.sets_list.size() << ", Tags: " << grammar.single_tags.size() << std::endl;

	if (grammar.rules_any) {
		std::cerr << grammar.rules_any->size() << " rules cannot be skipped by index." << std::endl;
	}

	if (grammar.has_dep) {
		std::cerr << "Grammar has dependency rules." << std::endl;
	}

	CG3::UStringMap* relabel_rules = new CG3::UStringMap;
	if (parse_relabel_file(relabel_rules, argv[2], locale_default, codepage_default, ux_stderr)) {
		std::cerr << "Error: Relabelling rule file could not be parsed - exiting!" << std::endl;
		CG3Quit(1);
	}

	UFILE *gout = u_fopen(argv[3], "w", locale_default, codepage_default);

	if (gout) {
		CG3::GrammarWriter writer(grammar, ux_stderr, relabel_rules);
		writer.writeGrammar(gout);
	}
	else {
		std::cerr << "Could not write grammar to " << argv[3] << std::endl;
	}

	delete relabel_rules;
	relabel_rules = 0;

	u_fclose(ux_stderr);

	u_cleanup();

	return status;
}


