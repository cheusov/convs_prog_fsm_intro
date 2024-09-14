LIBDEPS +=	libcommon:my_grep
LIBDEPS +=	libcommon:libc_grep
LIBDEPS +=	libcommon:tre_grep
#LIBDEPS +=	libcommon:rx_grep # librx is broken
LIBDEPS +=	libcommon:pcre2_grep
LIBDEPS +=	libcommon:onig_grep
LIBDEPS +=	libcommon:uxre_grep
LIBDEPS +=	libcommon:rxspencer_grep
LIBDEPS +=	libcommon:cppstl_grep
LIBDEPS +=	libcommon:re2_grep
LIBDEPS +=	libcommon:pire_grep
SUBPRJ  +=	presentation

INTERNALLIBS =	libcommon

.include "help.mk"

.include <mkc.mk>
