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

#include "regex.h"

#include <mkc_strlcpy.h>

#include <cstring>
#include <cassert>
#include <memory>
#include <string>

#include <pire/pire.h>

struct _regex_t {
	Pire::NonrelocScanner m_regex;
	std::string m_errmsg;
};

int pire_regcomp(regex_t *_preg, const char *regex, int cflags)
{
	_regex_t *preg = new _regex_t;
	_preg->p = preg;

	if (cflags != (REG_PIRE | REG_NOSUB)) {
		preg->m_errmsg = "Bad cflags";
		return REG_BAD_CFLAGS;
	}

	std::vector<Pire::wchar32> ucs4;
	Pire::Encodings::Utf8().FromLocal(regex, regex + strlen(regex), std::back_inserter(ucs4));

	preg->m_regex = Pire::Lexer(ucs4.begin(), ucs4.end())
		.SetEncoding(Pire::Encodings::Latin1())
		.Parse()
		.Surround()
		.Compile<Pire::NonrelocScanner>();
	// TODO: error check?

	return 0;
}

int pire_regexec(const regex_t* _preg, const char *string, size_t nmatch, regmatch_t *pmatch, int eflags)
{
	_regex_t *preg = (_regex_t*) (_preg->p);

	if (!nmatch) {
		//.Run(line, strlen(line));
		if (Pire::Runner(preg->m_regex).Begin().Run(string, string + strlen(string)).End())
			return 0;
		else
			return REG_NOMATCH;
	} else {
		abort(); // not supported
	}
}

size_t pire_regerror(int errcode, const regex_t* _preg, char *errbuf, size_t errbuf_size)
{
	_regex_t *preg = (_regex_t*) (_preg->p);
	strlcpy(errbuf, preg->m_errmsg.c_str(), errbuf_size);
	return preg->m_errmsg.size() + 1;
}

void pire_regfree(regex_t *_preg)
{
	_regex_t *preg = (_regex_t*) (_preg->p);
	_preg->p = nullptr;
	delete preg;
}
