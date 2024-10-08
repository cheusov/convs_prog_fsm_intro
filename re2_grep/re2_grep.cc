/*
 * Copyright (c) 2024 Aleksey Cheusov <vle@gmx.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <iostream>
#include <fstream>
#include <cstring>
#include <memory>

#include <re2/re2.h>

#include <getopt.h>

#include "readlines.h"

static std::unique_ptr<re2::RE2> regex;

static void reg_match(const char *line, size_t line_size)
{
	if (re2::RE2::PartialMatch(re2::StringPiece(line, line_size), *regex))
		puts(line);
}

static void reg_search(const char *line, size_t line_size)
{
//	std::string substring;
	re2::StringPiece found;
	if (re2::RE2::PartialMatch(
			re2::StringPiece(line, line_size), *regex, &found))
	{
		fwrite(found.data(), 1, found.size(), stdout);
		putchar('\n');
	}
}

static void usage(const char *progname)
{
	fprintf(stderr, "usage: %s [OPTIONS] REGEXP FILE\n\
OPTIONS:\n\
   -h            display this screen\n\
   -o            show only the part of a matching line that matches REGEXP\n\
\n", progname);
}

int main(int argc, char *argv[])
{
	int opt;
	int mode_search = 0;
	const char *progname = argv[0];

	while ((opt = getopt(argc, argv, "ho")) != -1) {
		switch (opt) {
			case 'h':
				usage(progname);
				exit(0);
				break;
			case 'o':
				mode_search = 1;
				break;
			default:
				usage(progname);
				exit(EXIT_FAILURE);
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <pattern> <filename>\n", progname);
		return 1;
	}

	const char *pattern = argv[0];
	const char *filename = argv[1];

	// Compile the regular expression
	regex = std::unique_ptr<re2::RE2>(new re2::RE2(pattern, re2::RE2::Quiet));
	if (!regex->ok()) {
		std::cerr << regex->error() << '\n';
		return 1;
	}

	if (mode_search)
		readlines2(reg_search, filename);
	else
		readlines2(reg_match, filename);

	return 0;
}
