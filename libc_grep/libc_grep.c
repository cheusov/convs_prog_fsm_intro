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

#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <getopt.h>

#include "readlines.h"

#ifndef REG_ONESUB /* defined by UXRE */
#define REG_ONESUB 0
#endif

static regex_t regex;

static void reg_match(const char *line)
{
	int ret = regexec(&regex, line, 0, NULL, 0);
	if (ret == 0) {
		// Match found, print the line
		puts(line);
	} else if (ret != REG_NOMATCH) {
		// Error occurred (other than no match)
		fprintf(stderr, "Regex match failed\n");
		exit(1);
	}
}

static void reg_search(const char *line)
{
	regmatch_t match;
	int ret = regexec(&regex, line, 1, &match, 0);
	if (ret == 0) {
		// Match found, print the substring
		fwrite(line + match.rm_so, 1, match.rm_eo - match.rm_so, stdout);
		putchar('\n');
	} else if (ret != REG_NOMATCH) {
		// Error occurred (other than no match)
		fprintf(stderr, "Regex match failed\n");
		exit(1);
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
	int ret;
	int opt;
	int regcomp_flags = REG_EXTENDED | REG_NOSUB;
	int subexpr = 0;
	const char *progname = argv[0];

	while ((opt = getopt(argc, argv, "ho")) != -1) {
		switch (opt) {
			case 'h':
				usage(progname);
				exit(0);
				break;
			case 'o':
				regcomp_flags &= ~REG_NOSUB;
				regcomp_flags |= REG_ONESUB;
				subexpr = 1;
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
	ret = regcomp(&regex, pattern, regcomp_flags);
	if (ret) {
		fprintf(stderr, "Could not compile regex\n");
		exit(1);
	}

	if (subexpr)
		readlines(reg_search, filename);
	else
		readlines(reg_match, filename);

	// Free regex memory
	regfree(&regex);

	return 0;
}
