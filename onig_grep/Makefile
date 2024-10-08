PROG =	onig_grep
SRCS =	libc_grep.c

MKC_CHECK_PKGCONFIG +=	oniguruma

.PATH:		../libc_grep
.NOPATH:	libc_grep.o

CPPFLAGS +=	-I.

CPPFLAGS +=	${ONIG_CPPFLAGS}
LDADD    +=	${ONIG_LDADD}
LDFLAGS  +=	${ONIG_LDFLAGS}

.include <mkc.mk>
