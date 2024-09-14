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

#include "file_match.h"

static regex_t regex;

static void match(const char *line)
{
	int reti = regexec(&regex, line, 0, NULL, 0);
	if (reti == 0) {
		// Match found, print the line
		puts(line);
	} else if (reti != REG_NOMATCH) {
		// Error occurred (other than no match)
		fprintf(stderr, "Regex match failed\n");
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	int reti;

	if (argc < 3) {
		fprintf(stderr, "Usage: %s <pattern> <filename>\n", argv[0]);
		return 1;
	}

	const char *pattern = argv[1];
	const char *filename = argv[2];

	// Compile the regular expression
	reti = regcomp(&regex, pattern, REG_EXTENDED | REG_NOSUB);
	if (reti) {
		fprintf(stderr, "Could not compile regex\n");
		exit(1);
	}

	file_match(match, filename);

	// Free regex memory
	regfree(&regex);

	return 0;
}
