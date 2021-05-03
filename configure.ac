dnl  Process this file with autoconf to produce a configure script.


dnl  ***************************************************************************
dnl     A U T O C O N F   E N V I R O N M E N T
dnl  ***************************************************************************


AC_PREREQ([2.69])

AC_INIT([Nautilus Annotations],
	[0.3.0],
	[madmurphy333@gmail.com],
	[nautilus-annotations],
	[https://gitlab.gnome.org/madmurphy/nautilus-annotations])

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_AUX_DIR([build-aux])

AC_CONFIG_SRCDIR([src/nautilus-annotations.c])

AC_CONFIG_HEADERS([config.h])

AC_CONFIG_FILES([Makefile
src/Makefile
po/Makefile.in])

AM_INIT_AUTOMAKE([1.11])

AM_SILENT_RULES([yes])

AC_PROG_CC

PKG_CHECK_MODULES([NAUTILUS_ANNOTATIONS],
	[glib-2.0 libnautilus-extension gtksourceview-4])

AC_CHECK_PROG([HAVE_PKGCONFIG], [pkg-config], [yes], [no])

AS_IF([test "x${HAVE_PKGCONFIG}" = xno],
	[AC_MSG_ERROR([you need to have pkgconfig installed!])])


dnl  ***************************************************************************
dnl     I N T E R N A T I O N A L I Z A T I O N
dnl  ***************************************************************************


IT_PROG_INTLTOOL([0.35.0])

AM_CONDITIONAL([NLS_ENABLED], [test "x${USE_NLS}" != xno])

AM_COND_IF([NLS_ENABLED], [
	AC_SUBST([GETTEXT_PACKAGE], AC_PACKAGE_TARNAME)
	AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],
		["${GETTEXT_PACKAGE}"], [GETTEXT package name])
	AM_GLIB_GNU_GETTEXT
	AM_GLIB_DEFINE_LOCALEDIR([NAUTILUS_ANNOTATIONS_LOCALEDIR])
])


dnl  ***************************************************************************
dnl     F I N A L I Z A T I O N
dnl  ***************************************************************************


AC_SUBST([NAUTILUS_EXTENSION_DIR],
	["$($PKG_CONFIG --variable=extensiondir libnautilus-extension)"])

LT_INIT([disable-static])

AC_OUTPUT


dnl  EOF

