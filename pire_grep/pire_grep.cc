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
#include <string>
#include <vector>

#include <pire/pire.h>

#include "readlines.h"

static Pire::NonrelocScanner regex;

static void match(const char *line)
{
	if (Pire::Runner(regex)//.Run(line, strlen(line));
			   .Begin()
			   .Run(line, line + strlen(line))
		.End())
	{
		puts(line);
	}
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		fprintf(stderr, "Usage: %s <pattern> <filename>\n", argv[0]);
		return 1;
	}

	const char *pattern = argv[1];
	const char *filename = argv[2];

	std::vector<Pire::wchar32> ucs4;
	Pire::Encodings::Utf8().FromLocal(pattern, pattern + strlen(pattern), std::back_inserter(ucs4));
	// Compile the regular expression
	regex = Pire::Lexer(ucs4.begin(), ucs4.end())
		.SetEncoding(Pire::Encodings::Latin1())
		.Parse()
		.Surround()
		.Compile<Pire::NonrelocScanner>();

	readlines(match, filename);

	return 0;
}
