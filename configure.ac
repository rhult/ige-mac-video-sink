dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.52)
 
AC_INIT(gtk-osx-video-sink, 0.1)
AC_CONFIG_SRCDIR(src/test.c)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

AM_MAINTAINER_MODE
AM_CONFIG_HEADER(config.h)

AM_DISABLE_STATIC
CXX=no
F77=no
AM_PROG_LIBTOOL

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC

AM_PATH_GLIB_2_0

dnl Defined in acinclude.m4
IMENDIO_COMPILE_WARNINGS

AG_GST_SET_PLUGINDIR

dnl  ------------------
dnl | Pkgconfig checks |---------------------------------------
dnl  ------------------

PKG_CHECK_MODULES(SINK, [
        glib-2.0 >= 2.12
	gtk+-2.0 >= 2.11
        gstreamer-0.10
        gstreamer-plugins-base-0.10
])

PKG_CHECK_MODULES(TEST, [
        glib-2.0 >= 2.12
	gtk+-2.0 >= 2.11
        gstreamer-0.10
])
dnl -----------------------------------------------------------

dnl This will cause the automake generated makefiles to pass the
dnl correct flags to aclocal.
ACLOCAL_AMFLAGS="\${ACLOCAL_FLAGS}"
AC_SUBST(ACLOCAL_AMFLAGS)

AC_OUTPUT([
	Makefile 
	src/Makefile
	data/Makefile
        data/gtk-osx-video-embed.pc
])

echo
echo "Prefix                 : $prefix"
echo
