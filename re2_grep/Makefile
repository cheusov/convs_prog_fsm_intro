PROG          =	re2_grep
SRCS          =	re2cc.cc libc_grep.c

.PATH:		../libc_grep
.NOPATH:	libc_grep.o

CPPFLAGS     +=	-I. # for regex.h

CPPFLAGS     +=	${RE2_CPPFLAGS}
LDADD        +=	${RE2_LDADD}
LDFLAGS      +=	${RE2_LDFLAGS}

MKC_FEATURES +=	strlcpy

.include <mkc.mk>
