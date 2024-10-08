PROG =	pcre2_grep
SRCS =	libc_grep.c

.PATH:		../libc_grep
.NOPATH:	libc_grep.o

CPPFLAGS +=	-I.

CPPFLAGS +=	${PCRE2_CPPFLAGS}
LDADD    +=	${PCRE2_LDADD}
LDFLAGS  +=	${PCRE2_LDFLAGS}

.include <mkc.mk>
