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

#include "stdafx.h"
#include "icu_uoptions.h"
#include "Recycler.h"
#include "Grammar.h"
#include "TextualParser.h"
#include "GrammarWriter.h"
#include "BinaryGrammar.h"
#include "GrammarApplicator.h"

#include "version.h"

#include "options.h"
using namespace Options;
void GAppSetOpts(CG3::GrammarApplicator *applicator);

int main(int argc, char* argv[]) {
	UFILE *ux_stdin = 0;
	UFILE *ux_stdout = 0;
	UFILE *ux_stderr = 0;

	clock_t main_timer = clock();

	UErrorCode status = U_ZERO_ERROR;
	srand((uint32_t)time(0));

	fprintf(stderr, "VISL CG-3 Disambiguator version %u.%u.%u.%u\n",
		CG3_VERSION_MAJOR, CG3_VERSION_MINOR, CG3_VERSION_PATCH, CG3_REVISION);
	U_MAIN_INIT_ARGS(argc, argv);

	argc = u_parseArgs(argc, argv, NUM_OPTIONS, options);

	if (argc < 0) {
		fprintf(stderr, "%s: error in command line argument \"%s\"\n", argv[0], argv[-argc]);
		return argc;
	}

	if (options[VERSION].doesOccur) {
		fprintf(stderr, "%s\n", CG3_COPYRIGHT_STRING);
		return U_ZERO_ERROR;
	}

	if (!options[GRAMMAR].doesOccur && !options[HELP1].doesOccur && !options[HELP2].doesOccur) {
		fprintf(stderr, "Error: No grammar specified - cannot continue!\n");
		argc = -argc;
	}

	if (argc < 0 || options[HELP1].doesOccur || options[HELP2].doesOccur) {
		fprintf(stderr, "Usage: vislcg3 [OPTIONS]\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "Options:\n");
		fprintf(stderr, " -h or -? or --help       Displays this list.\n");
		fprintf(stderr, " -V or --version          Prints version number.\n");
		fprintf(stderr, " -g or --grammar          Specifies the grammar file to use for disambiguation.\n");
		fprintf(stderr, " -p or --vislcg-compat    Tells the grammar compiler to be compatible with older VISLCG syntax.\n");
		fprintf(stderr, " --grammar-out            Writes the compiled grammar back out in textual form to a file.\n");
		fprintf(stderr, " --grammar-bin            Writes the compiled grammar back out in binary form to a file.\n");
		fprintf(stderr, " --grammar-info           Writes the compiled grammar back out in textual form to a file, with lots of statistics and information.\n");
		fprintf(stderr, " --grammar-only           Compiles the grammar only.\n");
		fprintf(stderr, " --trace                  Prints debug output alongside with normal output.\n");
		fprintf(stderr, " --trace-name-only        If a rule is named, omit the line number. Implies --trace.\n");
		fprintf(stderr, " --trace-no-removed       Does not print removed readings. Implies --trace.\n");
		fprintf(stderr, " --prefix                 Sets the prefix for mapping. Defaults to @.\n");
		fprintf(stderr, " --sections               Number of sections to run. Defaults to running all sections.\n");
		//fprintf(stderr, " --reorder                Rearranges rules so SELECTs are run first.\n");
		fprintf(stderr, " --single-run             Only runs each section once.\n");
		fprintf(stderr, " --no-mappings            Disables running any MAP, ADD, or REPLACE rules.\n");
		fprintf(stderr, " --no-corrections         Disables running any SUBSTITUTE or APPEND rules.\n");
		fprintf(stderr, " --no-before-sections     Disables running rules from BEFORE-SECTIONS.\n");
		fprintf(stderr, " --no-sections            Disables running rules from any SECTION.\n");
		fprintf(stderr, " --no-after-sections      Disables running rules from AFTER-SECTIONS.\n");
		fprintf(stderr, "\n");
		fprintf(stderr, " --num-windows            Number of windows to keep in before/ahead buffers. Defaults to 2.\n");
		fprintf(stderr, " --always-span            Forces all scanning tests to always span across window boundaries.\n");
		fprintf(stderr, " --soft-limit             Number of cohorts after which the SOFT-DELIMITERS kick in. Defaults to 300.\n");
		fprintf(stderr, " --hard-limit             Number of cohorts after which the window is delimited forcefully. Defaults to 500.\n");
		fprintf(stderr, " --no-magic-readings      Prevents running rules on magic readings.\n");
		fprintf(stderr, " --dep-allow-loops        Allows the creation of circular dependencies.\n");
		//fprintf(stderr, " --dep-delimit            Delimit via dependency information instead of DELIMITERS.\n");
		//fprintf(stderr, " --dep-reenum             Outputs the internal reenumeration of dependencies.\n");
		//fprintf(stderr, " --dep-humanize           Output dependency information in a more readable format.\n");
		fprintf(stderr, "\n");
		fprintf(stderr, " -O or --stdout           A file to print output to instead of stdout.\n");
		fprintf(stderr, " -I or --stdin            A file to read input from instead of stdin.\n");
		fprintf(stderr, " -E or --stderr           A file to print errors to instead of stderr.\n");
		fprintf(stderr, "\n");
		fprintf(stderr, " -C or --codepage-all     The codepage to use for grammar, input, and output streams. Auto-detects default from environment.\n");
		fprintf(stderr, " --codepage-grammar       Codepage to use for grammar. Overrides --codepage-all.\n");
		fprintf(stderr, " --codepage-input         Codepage to use for input. Overrides --codepage-all.\n");
		fprintf(stderr, " --codepage-output        Codepage to use for output and errors. Overrides --codepage-all.\n");
		fprintf(stderr, "\n");
		fprintf(stderr, " -L or --locale-all       The locale to use for grammar, input, and output streams. Defaults to en_US_POSIX.\n");
		fprintf(stderr, " --locale-grammar         Locale to use for grammar. Overrides --locale-all.\n");
		fprintf(stderr, " --locale-input           Locale to use for input. Overrides --locale-all.\n");
		fprintf(stderr, " --locale-output          Locale to use for output and errors. Overrides --locale-all.\n");

		return argc < 0 ? U_ILLEGAL_ARGUMENT_ERROR : U_ZERO_ERROR;
	}

	fflush(stderr);

	/* Initialize ICU */
	u_init(&status);
	if (U_FAILURE(status) && status != U_FILE_ACCESS_ERROR) {
		std::cerr << "Error: Cannot initialize ICU. Status = " << u_errorName(status) << std::endl;
		CG3Quit(1);
	}
	status = U_ZERO_ERROR;

	const char *codepage_default = ucnv_getDefaultName();
	const char *codepage_grammar = codepage_default;
	const char *codepage_input   = codepage_grammar;
	const char *codepage_output  = codepage_grammar;
	ucnv_setDefaultName("UTF-8");

	if (options[CODEPAGE_GRAMMAR].doesOccur) {
		codepage_grammar = options[CODEPAGE_GRAMMAR].value;
	} else if (options[CODEPAGE_ALL].doesOccur) {
		codepage_grammar = options[CODEPAGE_ALL].value;
	}

	if (options[CODEPAGE_INPUT].doesOccur) {
		codepage_input = options[CODEPAGE_INPUT].value;
	} else if (options[CODEPAGE_ALL].doesOccur) {
		codepage_input = options[CODEPAGE_ALL].value;
	}

	if (options[CODEPAGE_OUTPUT].doesOccur) {
		codepage_output = options[CODEPAGE_OUTPUT].value;
	} else if (options[CODEPAGE_ALL].doesOccur) {
		codepage_output = options[CODEPAGE_ALL].value;
	}

	std::cerr << "Codepage: default " << codepage_default << ", input " << codepage_input << ", output " << codepage_output << ", grammar " << codepage_grammar << std::endl;

	const char *locale_default = "en_US_POSIX"; //uloc_getDefault();
	const char *locale_grammar = locale_default;
	const char *locale_input   = locale_grammar;
	const char *locale_output  = locale_grammar;
	uloc_setDefault("en_US_POSIX", &status);

	if (options[LOCALE_GRAMMAR].doesOccur) {
		locale_grammar = options[LOCALE_GRAMMAR].value;
	} else if (options[LOCALE_ALL].doesOccur) {
		locale_grammar = options[LOCALE_ALL].value;
	}

	if (options[LOCALE_INPUT].doesOccur) {
		locale_input = options[LOCALE_INPUT].value;
	} else if (options[LOCALE_ALL].doesOccur) {
		locale_input = options[LOCALE_ALL].value;
	}

	if (options[LOCALE_OUTPUT].doesOccur) {
		locale_output = options[LOCALE_OUTPUT].value;
	} else if (options[LOCALE_ALL].doesOccur) {
		locale_output = options[LOCALE_ALL].value;
	}

	if (options[VERBOSE].doesOccur) {
		fprintf(stderr, "Locales: default %s, input %s, output %s, grammar %s\n", locale_default, locale_input, locale_output, locale_grammar);
	}

	bool stdin_isfile = false;
	bool stdout_isfile = false;
	bool stderr_isfile = false;

	if (!options[STDOUT].doesOccur) {
		ux_stdout = u_finit(stdout, locale_output, codepage_input);
	} else {
		stdout_isfile = true;
		ux_stdout = u_fopen(options[STDOUT].value, "wb", locale_output, codepage_output);
	}
	if (!ux_stdout) {
		std::cerr << "Error: Failed to open the output stream for writing!" << std::endl;
		CG3Quit(1);
	}

	if (!options[STDERR].doesOccur) {
		ux_stderr = u_finit(stderr, locale_output, codepage_input);
	} else {
		stderr_isfile = true;
		ux_stderr = u_fopen(options[STDERR].value, "wb", locale_output, codepage_output);
	}
	if (!ux_stdout) {
		std::cerr << "Error: Failed to open the error stream for writing!" << std::endl;
		CG3Quit(1);
	}

	if (!options[STDIN].doesOccur) {
		ux_stdin = u_finit(stdin, locale_input, codepage_input);
	} else {
		struct stat info;
		int serr = stat(options[STDIN].value, &info);
		if (serr) {
			std::cerr << "Error: Cannot stat " << options[STDIN].value << " due to error " << serr << "!" << std::endl;
			return serr;
		}
		stdin_isfile = true;
		ux_stdin = u_fopen(options[STDIN].value, "rb", locale_input, codepage_input);
	}
	if (!ux_stdin) {
		std::cerr << "Error: Failed to open the input stream for reading!" << std::endl;
		CG3Quit(1);
	}

	CG3::Recycler::instance();
	init_gbuffers();
	init_strings();
	init_keywords();
	init_regexps(ux_stderr);
	CG3::Grammar *grammar = new CG3::Grammar();

	CG3::IGrammarParser *parser = 0;
	FILE *input = fopen(options[GRAMMAR].value, "rb");
	if (!input) {
		std::cerr << "Error: Error opening " << options[GRAMMAR].value << " for reading!" << std::endl;
		CG3Quit(1);
	}
	fread(cbuffers[0], 1, 4, input);
	fclose(input);

	if (cbuffers[0][0] == 'C' && cbuffers[0][1] == 'G' && cbuffers[0][2] == '3' && cbuffers[0][3] == 'B') {
		std::cerr << "Info: Binary grammar detected." << std::endl;
		parser = new CG3::BinaryGrammar(grammar, ux_stderr);
	}
	else {
		parser = new CG3::TextualParser(ux_stdin, ux_stdout, ux_stderr);
	}
	if (options[VERBOSE].doesOccur) {
		if (options[VERBOSE].value) {
			parser->setVerbosity(abs(atoi(options[VERBOSE].value)));
		}
		else {
			parser->setVerbosity(1);
		}
	}
	grammar->ux_stderr = ux_stderr;
	CG3::Tag *tag_any = grammar->allocateTag(stringbits[S_ASTERIK]);
	tag_any = grammar->addTag(tag_any);
	grammar->tag_any = tag_any->hash;
	parser->setResult(grammar);
	parser->setCompatible(options[VISLCGCOMPAT].doesOccur != 0);

	if (options[VERBOSE].doesOccur) {
		std::cerr << "Initialization took " << (clock()-main_timer)/(double)CLOCKS_PER_SEC << " seconds." << std::endl;
	}
	main_timer = clock();

	if (parser->parse_grammar_from_file(options[GRAMMAR].value, locale_grammar, codepage_grammar)) {
		std::cerr << "Error: Grammar could not be parsed - exiting!" << std::endl;
		CG3Quit(1);
	}

	if (options[MAPPING_PREFIX].doesOccur) {
		grammar->mapping_prefix = options[MAPPING_PREFIX].value[0];
	}
	if (options[VERBOSE].doesOccur) {
		std::cerr << "Reindexing grammar..." << std::endl;
	}
	grammar->reindex();

	delete parser;
	parser = 0;
	
	std::cerr << "Parsing grammar took " << (clock()-main_timer)/(double)CLOCKS_PER_SEC << " seconds." << std::endl;
	main_timer = clock();

	std::cerr << "Grammar has " << grammar->sections.size() << " sections, " << grammar->rule_by_line.size() << " rules, " << grammar->sets_by_contents.size() << " sets, " << grammar->single_tags.size() << " tags." << std::endl;
	if (grammar->rules_by_tag.find(tag_any->hash) != grammar->rules_by_tag.end()) {
		std::cerr << grammar->rules_by_tag.find(tag_any->hash)->second->size() << " rules cannot be skipped by index." << std::endl;
	}
	if (grammar->has_dep) {
		std::cerr << "Grammar has dependency rules." << std::endl;
	}

	if (grammar->is_binary) {
		if (options[GRAMMAR_INFO].doesOccur) {
			std::cerr << "Error: Re-ordering statistics cannot be gathered with a binary grammar." << std::endl;
			CG3Quit(1);
		}
		if (options[GRAMMAR_BIN].doesOccur || options[GRAMMAR_OUT].doesOccur) {
			std::cerr << "Error: Binary grammars cannot be rewritten." << std::endl;
			CG3Quit(1);
		}
		if (options[STATISTICS].doesOccur) {
			std::cerr << "Error: Statistics cannot be gathered with a binary grammar." << std::endl;
			CG3Quit(1);
		}
	}

	if (options[GRAMMAR_INFO].doesOccur && !stdin_isfile) {
		std::cerr << "Error: Re-ordering statistics can only be gathered with file input option (-I, --stdin) as the file must be re-run multiple times." << std::endl;
		CG3Quit(1);
	}

	if (!options[CHECK_ONLY].doesOccur && !options[GRAMMAR_ONLY].doesOccur) {
		CG3::GrammarApplicator *applicator = new CG3::GrammarApplicator(ux_stdin, ux_stdout, ux_stderr);
		applicator->setGrammar(grammar);
		GAppSetOpts(applicator);
		applicator->runGrammarOnText(ux_stdin, ux_stdout);
		delete applicator;
		applicator = 0;

		if (options[VERBOSE].doesOccur) {
			std::cerr << "Applying grammar on input took " << (clock()-main_timer)/(double)CLOCKS_PER_SEC << " seconds." << std::endl;
		}
		main_timer = clock();
	}

	if (options[GRAMMAR_INFO].doesOccur) {
		UFILE *gout = u_fopen(options[GRAMMAR_INFO].value, "wb", locale_output, codepage_output);
		if (gout) {
			grammar->resetStatistics();

			if (!options[CHECK_ONLY].doesOccur && !options[GRAMMAR_ONLY].doesOccur) {
				u_frewind(ux_stdin);

				CG3::GrammarApplicator *applicator = new CG3::GrammarApplicator(ux_stdin, ux_stdout, ux_stderr);
				applicator->setGrammar(grammar);
				GAppSetOpts(applicator);
				applicator->enableStatistics();
				applicator->runGrammarOnText(ux_stdin, ux_stdout);
				delete applicator;
				applicator = 0;

				if (options[VERBOSE].doesOccur) {
					std::cerr << "Applying context-sorted grammar on input took " << (clock()-main_timer)/(double)CLOCKS_PER_SEC << " seconds." << std::endl;
				}
				main_timer = clock();
			}

			/*
			for (uint32_t j=0;j<grammar->rules.size();j++) {
				grammar->rules[j]->reweight();
			}
			for (uint32_t i=0;i<grammar->sections.size()-1;i++) {
				std::sort(&grammar->rules[grammar->sections[i]], &grammar->rules[grammar->sections[i+1]-1], CG3::Rule::cmp_quality);
			}
			//*/

			CG3::GrammarWriter *writer = new CG3::GrammarWriter(grammar, ux_stderr);
			writer->statistics = true;
			writer->writeGrammar(gout);
			delete writer;
			writer = 0;

			if (options[VERBOSE].doesOccur) {
				std::cerr << "Writing textual grammar with statistics took " << (clock()-main_timer)/(double)CLOCKS_PER_SEC << " seconds." << std::endl;
			}
			main_timer = clock();
		} else {
			std::cerr << "Could not write grammar to " << options[GRAMMAR_INFO].value << std::endl;
		}

		if (!options[CHECK_ONLY].doesOccur && !options[GRAMMAR_ONLY].doesOccur) {
			u_frewind(ux_stdin);

			CG3::GrammarApplicator *applicator = new CG3::GrammarApplicator(ux_stdin, ux_stdout, ux_stderr);
			applicator->setGrammar(grammar);
			GAppSetOpts(applicator);
			applicator->enableStatistics();
			applicator->runGrammarOnText(ux_stdin, ux_stdout);
			delete applicator;
			applicator = 0;

			if (options[VERBOSE].doesOccur) {
				std::cerr << "Applying fully-sorted grammar on input took " << (clock()-main_timer)/(double)CLOCKS_PER_SEC << " seconds." << std::endl;
			}
			main_timer = clock();
		}
	}

	if (options[GRAMMAR_OUT].doesOccur) {
		UFILE *gout = u_fopen(options[GRAMMAR_OUT].value, "w", locale_output, codepage_output);
		if (gout) {
			CG3::GrammarWriter *writer = new CG3::GrammarWriter(grammar, ux_stderr);
			if (options[STATISTICS].doesOccur) {
				writer->statistics = true;
			}
			writer->writeGrammar(gout);
			delete writer;
			writer = 0;

			if (options[VERBOSE].doesOccur) {
				std::cerr << "Writing textual grammar took " << (clock()-main_timer)/(double)CLOCKS_PER_SEC << " seconds." << std::endl;
			}
			main_timer = clock();
		} else {
			std::cerr << "Could not write grammar to " << options[GRAMMAR_OUT].value << std::endl;
		}
	}

	if (options[GRAMMAR_BIN].doesOccur) {
		FILE *gout = fopen(options[GRAMMAR_BIN].value, "wb");
		if (gout) {
			CG3::BinaryGrammar *writer = new CG3::BinaryGrammar(grammar, ux_stderr);
			writer->writeBinaryGrammar(gout);
			delete writer;
			writer = 0;

			if (options[VERBOSE].doesOccur) {
				std::cerr << "Writing binary grammar took " << (clock()-main_timer)/(double)CLOCKS_PER_SEC << " seconds." << std::endl;
			}
			main_timer = clock();
		} else {
			std::cerr << "Could not write grammar to " << options[GRAMMAR_BIN].value << std::endl;
		}
	}

	u_fclose(ux_stdin);
	u_fclose(ux_stdout);
	u_fclose(ux_stderr);

	delete grammar;
	grammar = 0;

	free_strings();
	free_keywords();
	free_regexps();
	free_gbuffers();

	CG3::Recycler::cleanup();

	u_cleanup();

	if (options[VERBOSE].doesOccur) {
		std::cerr << "Cleanup took " << (clock()-main_timer)/(double)CLOCKS_PER_SEC << " seconds." << std::endl;
	}

	return status;
}

void CG3Quit(const int32_t c, const char* file, const uint32_t line) {
	if (file && line) {
		std::cerr << std::flush;
		std::cerr << "CG3Quit triggered from " << file << " line " << line << "." << std::endl;
	}
	exit(c);
}

void GAppSetOpts(CG3::GrammarApplicator *applicator) {
	if (options[ALWAYS_SPAN].doesOccur) {
		applicator->always_span = true;
	}
	applicator->apply_mappings = true;
	if (options[NOMAPPINGS].doesOccur) {
		applicator->apply_mappings = false;
	}
	applicator->apply_corrections = true;
	if (options[NOCORRECTIONS].doesOccur) {
		applicator->apply_corrections = false;
	}
	applicator->no_before_sections = false;
	if (options[NOBEFORESECTIONS].doesOccur) {
		applicator->no_before_sections = true;
	}
	applicator->no_sections = false;
	if (options[NOSECTIONS].doesOccur) {
		applicator->no_sections = true;
	}
	applicator->no_after_sections = false;
	if (options[NOAFTERSECTIONS].doesOccur) {
		applicator->no_after_sections = true;
	}
	if (options[TRACE].doesOccur) {
		applicator->trace = true;
	}
	if (options[TRACE_NAME_ONLY].doesOccur) {
		applicator->trace = true;
		applicator->trace_name_only = true;
	}
	if (options[TRACE_NO_REMOVED].doesOccur) {
		applicator->trace = true;
		applicator->trace_no_removed = true;
	}
	if (options[SINGLERUN].doesOccur) {
		applicator->single_run = true;
	}
	if (options[GRAMMAR_INFO].doesOccur) {
		applicator->enableStatistics();
	}
	if (options[STATISTICS].doesOccur) {
		applicator->enableStatistics();
	}
	if (options[SECTIONS].doesOccur) {
		applicator->sections = abs(atoi(options[SECTIONS].value));
	}
	if (options[VERBOSE].doesOccur) {
		if (options[VERBOSE].value) {
			applicator->verbosity_level = abs(atoi(options[VERBOSE].value));
		}
		else {
			applicator->verbosity_level = 1;
		}
	}
	if (options[NUM_WINDOWS].doesOccur) {
		applicator->num_windows = abs(atoi(options[NUM_WINDOWS].value));
	}
	if (options[SOFT_LIMIT].doesOccur) {
		applicator->soft_limit = abs(atoi(options[SOFT_LIMIT].value));
	}
	if (options[HARD_LIMIT].doesOccur) {
		applicator->hard_limit = abs(atoi(options[HARD_LIMIT].value));
	}
	if (options[DEP_REENUM].doesOccur) {
		applicator->dep_reenum = true;
	}
	if (options[DEP_HUMANIZE].doesOccur) {
		applicator->dep_reenum = true;
		applicator->dep_humanize = true;
	}
	if (options[DEP_ORIGINAL].doesOccur) {
		applicator->dep_original = true;
	}
	if (options[DEP_ALLOW_LOOPS].doesOccur) {
		applicator->dep_block_loops = false;
	}
	if (options[MAGIC_READINGS].doesOccur) {
		applicator->allow_magic_readings = false;
	}
}
