dnl  Process this file with autoconf to produce a configure script.


dnl  **************************************************************************
dnl     A U T O C O N F   E N V I R O N M E N T
dnl  **************************************************************************


AC_PREREQ([2.69])

AC_INIT([Nautilus Annotations],
	[0.10.0],
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

AC_PROG_GREP

AC_CHECK_PROG([HAVE_PKGCONFIG], [pkg-config], [yes], [no])

AS_IF([test "x${HAVE_PKGCONFIG}" = xno],
	[AC_MSG_ERROR([you need to have pkgconfig installed!])])

PKG_CHECK_MODULES([NAUTILUS_ANNOTATIONS],
	[glib-2.0 gtk4 gtksourceview-5 libnautilus-extension-4])

AC_ARG_VAR([FIND], [Unix find utility])

AS_IF([test "x${FIND}" = x], [AC_PATH_PROG([FIND], [find])])

AM_CONDITIONAL([HAVE_FIND], [test "x${FIND}" != x])


dnl  **************************************************************************
dnl     C S S   T H E M E S
dnl  **************************************************************************

m4_define([NA_THEMES_DIR], [themes])

m4_define([NA_DEFAULT_THEME], [notebook])

dnl  Build the theme list once at autoreconf time via `m4_esyscmd()` + `find`
m4_define([NA_CSS_THEMES],
	m4_dquote(m4_esyscmd([find ']NA_THEMES_DIR[' -name '*-theme.css' \
	-printf '[%P///],' | sed 's/-[^-]*.css\/\/\///g;s/,$//'])))

AC_ARG_WITH([theme],
	[AS_HELP_STRING([--with-theme=THEME-NAME],
		[install a specific CSS theme for this extension; possible
		values for THEME-NAME are: ]m4_map_args_sep(["], ["], [[, ]],
		NA_CSS_THEMES)[ - use "no" @{:@--without-theme@:}@ for not
		installing any theme; please bear in mind that
		extension-specific themes have always higher priority over GTK
		themes; if you want to style this extension via the latter or
		simply want to experiment with CSS globally you should not
		install any theme @{:@--without-theme@:}@
		@<:@default=]NA_DEFAULT_THEME[@:>@])],
	[AS_CASE(["x${withval}"],
		m4_quote(m4_map_args_sep(['x], ['], [|], NA_CSS_THEMES)),
			[AS_VAR_SET([CSS_THEME],
				['@S|@@{:@top_srcdir@:}@/]NA_THEMES_DIR[/'"${withval}"'-theme.css'])],
		['x'|'xyes'],
			[AC_MSG_ERROR([a theme name must be specified for option --with-theme=...])],
		[*],
			[AC_MSG_ERROR([theme "${withval}" not found])])],
	[AS_VAR_SET([CSS_THEME],
		['@S|@@{:@top_srcdir@:}@/]NA_THEMES_DIR[/]NA_DEFAULT_THEME[-theme.css'])])


dnl  **************************************************************************
dnl     I N T E R N A T I O N A L I Z A T I O N
dnl  **************************************************************************

AM_GNU_GETTEXT([external])

AM_GNU_GETTEXT_VERSION([0.19])

AM_CONDITIONAL([NLS_ENABLED], [test "x${USE_NLS}" != xno])

AM_COND_IF([NLS_ENABLED], [
	AC_SUBST([GETTEXT_PACKAGE], AC_PACKAGE_TARNAME)
	AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],
		["${GETTEXT_PACKAGE}"], [GETTEXT package name])
])


dnl  **************************************************************************
dnl     F I N A L I Z A T I O N
dnl  **************************************************************************

AC_ARG_WITH([nautilus-libdir],
	[AS_HELP_STRING([--without-nautilus-libdir],
		[ignore the <libdir> variable used by libnautilus-extension-4
		and and use the current <libdir> instead @<:@default=no@:>@])],
	[:],
	[AS_VAR_SET([with_nautilus_libdir], ['yes'])])

AS_IF([test "x${with_nautilus_libdir}" = xno],
	[AS_VAR_SET([NAUTILUS_EXTENSION_DIR], m4_normalize(["$(
		"${PKG_CONFIG}" --define-variable='libdir=${libdir}'
			--variable=extensiondir libnautilus-extension-4
	)"]))],
	[AS_VAR_SET([NAUTILUS_EXTENSION_DIR], m4_normalize(["$(
		"${PKG_CONFIG}" --variable=extensiondir libnautilus-extension-4
	)"]))])

AC_SUBST([NAUTILUS_EXTENSION_DIR])

AC_SUBST([CSS_THEMES],
	[']m4_map_args_sep(NA_THEMES_DIR[/],
		[-theme.css], [ ], NA_CSS_THEMES)['])

AM_SUBST_NOTMAKE([CSS_THEMES])

AC_SUBST([CSS_THEME])

AC_DEFINE([STYLESHEET_FILENAME], ["style.css"], [Stylesheet file name])

LT_INIT([disable-static])

AC_OUTPUT


dnl  EOF

