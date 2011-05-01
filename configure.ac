AC_PREREQ(2.50)
AC_INIT(Codeblocks Debugger GDBMI, 0.1, fuscated@gmail.com)

AM_INIT_AUTOMAKE([subdir-objects])

LT_PREREQ([2.2])
LT_INIT([dlopen])

#AM_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile])


# Checks for programs.
#AC_PROG_CC
AC_PROG_CXX
# Checks for libraries.

AM_OPTIONS_WXCONFIG
AM_PATH_WXCONFIG(2.8.0, wxWin=1)
	if test "$wxWin" != 1; then
		AC_MSG_ERROR([
			wxWidgets must be installed on your system.

			Please check that wx-config is in path, the directory
			where wxWidgets libraries are installed (returned by
			'wx-config --libs' or 'wx-config --static --libs' command)
			is in LD_LIBRARY_PATH or equivalent variable and
			wxWindows version is 2.8.0 or above.
			])
	fi

PKG_CHECK_MODULES([CODEBLOCKS], [codeblocks])

AC_SUBST([CODEBLOCKS_CFLAGS])
AC_SUBST([CODEBLOCKS_LIBS])

#cb_plugin_datadir="`$PKG_CONFIG codeblocks --variable plugindir`"
cb_plugin_datadir="`$PKG_CONFIG codeblocks --variable sharedir`/codeblocks/"
cb_plugin_libdir="`$PKG_CONFIG codeblocks --variable libdir`/codeblocks/plugins/"

AC_SUBST([cb_plugin_datadir])
AC_SUBST([cb_plugin_libdir])

CPPFLAGS="$CPPFLAGS $CODEBLOCKS_CFLAGS"
LIBS="$LIBS $CODEBLOCKS_LIBS"


# Checks for header files.
# Checks for typedefs, structures, and compiler characteristics.
# Checks for library functions.

AC_SUBST(CPPFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(CFLAGS)
AC_SUBST(LIBS)

AC_OUTPUT