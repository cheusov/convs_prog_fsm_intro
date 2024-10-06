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
SUBPRJ  +=	perl_grep ruby_grep python_grep
SUBPRJ  +=	presentation
SUBPRJ  +=	tests/map_uint_to_uint

INTERNALLIBS =	libcommon

# do not run tests for most projects
NODEPS  +=	test-presentation:* test-libc_grep:* test-tre_grep:* \
    test-pcre2_grep:* test-onig_grep:* test-uxre_grep:* \
    test-rxspencer_grep:* test-cppstl_grep:* test-re2_grep:* \
    test-pire_grep:*  test-perl_grep:*  test-ruby_grep:* \
    test-python_grep:*  test-libcommon:*

# do not install test executable
NODEPS  +=	install-tests/map_uint_to_uint:*

# before testing, build appropriate projects
test: all-my_grep all-map_uint_to_uint

#
.include "help.mk"
.include <mkc.mk>
