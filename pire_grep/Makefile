PROG      =	pire_grep
SRCS      =	pirecc.cc libc_grep.c

.PATH:		../libc_grep
.NOPATH:	libc_grep.o

CPPFLAGS +=	-I. # for regex.h

CPPFLAGS +=	${PIRE_CPPFLAGS}
LDADD    +=	${PIRE_LDADD}
LDFLAGS  +=	${PIRE_LDFLAGS}

MKC_FEATURES +=	strlcpy

.include <mkc.mk>
