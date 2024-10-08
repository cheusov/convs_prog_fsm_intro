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

#include <re2/re2.h>

struct _regex_t {
	std::unique_ptr<re2::RE2> m_regex;
	std::string m_errmsg;
};

int re2_regcomp(regex_t *_preg, const char *regex, int cflags)
{
	_regex_t *preg = new _regex_t;
	_preg->p = preg;

	if (cflags != REG_RE2 && cflags != (REG_RE2 | REG_NOSUB)) {
		preg->m_errmsg = "Bad cflags";
		return REG_BAD_CFLAGS;
	}

	std::string regexp = regex;
	if ((cflags & REG_NOSUB) == 0)
		regexp = "(" + regexp + ")";

	preg->m_regex = std::unique_ptr<re2::RE2>(new re2::RE2(regexp, re2::RE2::Quiet));
	if (!preg->m_regex->ok()) {
		std::string err = preg->m_regex->error();
		preg->m_errmsg = err;
		return REG_COMP_FAILED;
	}

	return 0;
}

int re2_regexec(const regex_t* _preg, const char *string, size_t nmatch, regmatch_t *pmatch, int eflags)
{
	_regex_t *preg = (_regex_t*) (_preg->p);

	if (!nmatch) {
		if (re2::RE2::PartialMatch(re2::StringPiece(string, strlen(string)), *preg->m_regex))
			return 0;
		else
			return REG_NOMATCH;
	} else {
//		assert(nmatch == 1); // the only supported mode

		re2::StringPiece found;
		if (re2::RE2::PartialMatch(
				re2::StringPiece(string, strlen(string)), *preg->m_regex, &found))
		{
			size_t start = found.data() - string;
			pmatch->rm_so = start;
			pmatch->rm_eo = start + found.size();
			return 0;
		} else {
			return REG_NOMATCH;
		}
	}
}

size_t re2_regerror(int errcode, const regex_t* _preg, char *errbuf, size_t errbuf_size)
{
	_regex_t *preg = (_regex_t*) (_preg->p);
	strlcpy(errbuf, preg->m_errmsg.c_str(), errbuf_size);
	return preg->m_errmsg.size() + 1;
}

void re2_regfree(regex_t *_preg)
{
	_regex_t *preg = (_regex_t*) (_preg->p);
	_preg->p = nullptr;
	delete preg;
}
