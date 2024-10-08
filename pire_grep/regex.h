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

#ifndef LIBPIRECC_REGEX_H
#define LIBPIRECC_REGEX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

// regcomp flags
#define REG_PIRE    0x800
#define REG_NOSUB   0x400

// errors
#define REG_NOMATCH        21

#define REG_COMP_FAILED    22
#define REG_BAD_CFLAGS     23

typedef struct
{
	void * p;
} regex_t;

typedef ssize_t regoff_t;

typedef struct
{
	regoff_t	rm_so;
	regoff_t	rm_eo;
} regmatch_t;

int	pire_regcomp(regex_t *, const char *, int);
int	pire_regexec(const regex_t *, const char *, size_t, regmatch_t *, int);
size_t	pire_regerror(int, const regex_t *, char *, size_t);
void	pire_regfree(regex_t *);

#define regcomp(preg, regex, cflags) pire_regcomp(preg, regex, cflags)
#define regexec(preg, string, nmatch, pmatch, eflags) pire_regexec(preg, string, nmatch, pmatch, eflags)
#define regerror(errcode, preg, errbuf, errbuf_size) pire_regerror(errcode, preg, errbuf, errbuf_size)
#define regfree(preg) pire_regfree(preg)

#ifdef __cplusplus
}
#endif

#endif /* LIBPIRECC_REGEX_H */
