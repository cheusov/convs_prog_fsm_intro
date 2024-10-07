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
#include <string.h>
#include <stdlib.h>

#include "readlines.h"

void readlines(void (*process_line)(const char *), const char *filename)
{
	int reti;
	char *line = NULL;
	size_t len;
	size_t line_len;
	ssize_t nread;
	FILE *file = NULL;

	if (!strcmp(filename, "-")) {
		file = stdin;
	} else {
		// Open the file
		file = fopen(filename, "r");
		if (!file) {
			fprintf(stderr, "Could not open file: %s\n", filename);
			exit(1);
		}
	}

	// Read each line from the file and search for the pattern
	while ((nread = getline(&line, &len, file)) != -1) {
		// Remove newline character if present
		line_len = strlen(line);
		if (line[line_len - 1] == '\n') {
			line[line_len - 1] = '\0';
		}

		// Process line
		process_line(line);
	}

	// Close the file
	if (file != stdin)
		fclose(file);
}

void readlines2(void (*process_line)(const char *, size_t), const char *filename)
{
	int reti;
	char *line = NULL;
	size_t len;
	size_t line_len;
	ssize_t nread;
	FILE *file = NULL;

	if (!strcmp(filename, "-")) {
		file = stdin;
	} else {
		// Open the file
		file = fopen(filename, "r");
		if (!file) {
			fprintf(stderr, "Could not open file: %s\n", filename);
			exit(1);
		}
	}

	// Read each line from the file and search for the pattern
	while ((nread = getline(&line, &len, file)) != -1) {
		// Remove newline character if present
		line_len = strlen(line);
		if (line[line_len - 1] == '\n') {
			line[--line_len] = '\0';
		}

		// Process line
		process_line(line, line_len);
	}

	// Close the file
	if (file != stdin)
		fclose(file);
}
