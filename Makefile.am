# Process this file with automake to produce Makefile.in


ACLOCAL_AMFLAGS = -I m4

AM_DISTCHECK_CONFIGURE_FLAGS = --without-nautilus-libdir

SUBDIRS = \
	src

dist_doc_DATA = \
	AUTHORS \
	COPYING \
	ChangeLog \
	NEWS \
	README \
	docs/shortcuts.md \
	docs/styling.md

themesdir = $(docdir)/css-factory
dist_themes_DATA = \
	themes/README.md \
	@CSS_THEMES@

nodist_pkgdata_DATA = \
	no-dist/style.css

# Launch
#
#     echo "nobase_dist_data_DATA =$(find icons -type f -printf ' \\\n\t%p')"
#
# to get this list
nobase_dist_data_DATA = \
	icons/Adwaita/64x64/actions/unannotate-symbolic.png \
	icons/Adwaita/64x64/actions/annotate-symbolic.png \
	icons/Adwaita/64x64/emblems/emblem-annotations-symbolic.png \
	icons/Adwaita/64x64/emblems/emblem-annotations.png \
	icons/Adwaita/24x24/actions/unannotate-symbolic.png \
	icons/Adwaita/24x24/actions/annotate-symbolic.png \
	icons/Adwaita/24x24/emblems/emblem-annotations-symbolic.png \
	icons/Adwaita/24x24/emblems/emblem-annotations.png \
	icons/Adwaita/48x48/actions/unannotate-symbolic.png \
	icons/Adwaita/48x48/actions/annotate-symbolic.png \
	icons/Adwaita/48x48/emblems/emblem-annotations-symbolic.png \
	icons/Adwaita/48x48/emblems/emblem-annotations.png \
	icons/Adwaita/96x96/actions/unannotate-symbolic.png \
	icons/Adwaita/96x96/actions/annotate-symbolic.png \
	icons/Adwaita/96x96/emblems/emblem-annotations-symbolic.png \
	icons/Adwaita/96x96/emblems/emblem-annotations.png \
	icons/Adwaita/scalable/actions/annotate-symbolic.svg \
	icons/Adwaita/scalable/actions/unannotate-symbolic.svg \
	icons/Adwaita/scalable/emblems/emblem-annotations-symbolic.svg \
	icons/Adwaita/scalable/emblems/emblem-annotations.svg \
	icons/Adwaita/22x22/actions/unannotate-symbolic.png \
	icons/Adwaita/22x22/actions/annotate-symbolic.png \
	icons/Adwaita/22x22/emblems/emblem-annotations-symbolic.png \
	icons/Adwaita/22x22/emblems/emblem-annotations.png \
	icons/Adwaita/256x256/actions/unannotate-symbolic.png \
	icons/Adwaita/256x256/actions/annotate-symbolic.png \
	icons/Adwaita/256x256/emblems/emblem-annotations-symbolic.png \
	icons/Adwaita/256x256/emblems/emblem-annotations.png \
	icons/Adwaita/8x8/actions/unannotate-symbolic.png \
	icons/Adwaita/8x8/actions/annotate-symbolic.png \
	icons/Adwaita/8x8/emblems/emblem-annotations-symbolic.png \
	icons/Adwaita/8x8/emblems/emblem-annotations.png \
	icons/Adwaita/16x16/actions/unannotate-symbolic.png \
	icons/Adwaita/16x16/actions/annotate-symbolic.png \
	icons/Adwaita/16x16/emblems/emblem-annotations-symbolic.png \
	icons/Adwaita/16x16/emblems/emblem-annotations.png \
	icons/Adwaita/32x32/actions/unannotate-symbolic.png \
	icons/Adwaita/32x32/actions/annotate-symbolic.png \
	icons/Adwaita/32x32/emblems/emblem-annotations-symbolic.png \
	icons/Adwaita/32x32/emblems/emblem-annotations.png \
	icons/Adwaita/512x512/actions/unannotate-symbolic.png \
	icons/Adwaita/512x512/actions/annotate-symbolic.png \
	icons/Adwaita/512x512/emblems/emblem-annotations-symbolic.png \
	icons/Adwaita/512x512/emblems/emblem-annotations.png \
	icons/hicolor/64x64/actions/unannotate-symbolic.png \
	icons/hicolor/64x64/actions/annotate-symbolic.png \
	icons/hicolor/64x64/actions/annotate.png \
	icons/hicolor/64x64/actions/unannotate.png \
	icons/hicolor/64x64/emblems/emblem-annotations.png \
	icons/hicolor/24x24/actions/unannotate-symbolic.png \
	icons/hicolor/24x24/actions/annotate-symbolic.png \
	icons/hicolor/24x24/actions/annotate.png \
	icons/hicolor/24x24/actions/unannotate.png \
	icons/hicolor/24x24/emblems/emblem-annotations.png \
	icons/hicolor/48x48/actions/unannotate-symbolic.png \
	icons/hicolor/48x48/actions/annotate-symbolic.png \
	icons/hicolor/48x48/actions/annotate.png \
	icons/hicolor/48x48/actions/unannotate.png \
	icons/hicolor/48x48/emblems/emblem-annotations.png \
	icons/hicolor/96x96/actions/unannotate-symbolic.png \
	icons/hicolor/96x96/actions/annotate-symbolic.png \
	icons/hicolor/96x96/actions/annotate.png \
	icons/hicolor/96x96/actions/unannotate.png \
	icons/hicolor/96x96/emblems/emblem-annotations.png \
	icons/hicolor/scalable/actions/annotate-symbolic.svg \
	icons/hicolor/scalable/actions/unannotate-symbolic.svg \
	icons/hicolor/scalable/actions/annotate.svg \
	icons/hicolor/scalable/actions/unannotate.svg \
	icons/hicolor/scalable/emblems/emblem-annotations.svg \
	icons/hicolor/22x22/actions/unannotate-symbolic.png \
	icons/hicolor/22x22/actions/annotate-symbolic.png \
	icons/hicolor/22x22/actions/annotate.png \
	icons/hicolor/22x22/actions/unannotate.png \
	icons/hicolor/22x22/emblems/emblem-annotations.png \
	icons/hicolor/256x256/actions/unannotate-symbolic.png \
	icons/hicolor/256x256/actions/annotate-symbolic.png \
	icons/hicolor/256x256/actions/annotate.png \
	icons/hicolor/256x256/actions/unannotate.png \
	icons/hicolor/256x256/emblems/emblem-annotations.png \
	icons/hicolor/8x8/actions/unannotate-symbolic.png \
	icons/hicolor/8x8/actions/annotate-symbolic.png \
	icons/hicolor/8x8/actions/annotate.png \
	icons/hicolor/8x8/actions/unannotate.png \
	icons/hicolor/8x8/emblems/emblem-annotations.png \
	icons/hicolor/16x16/actions/unannotate-symbolic.png \
	icons/hicolor/16x16/actions/annotate-symbolic.png \
	icons/hicolor/16x16/actions/annotate.png \
	icons/hicolor/16x16/actions/unannotate.png \
	icons/hicolor/16x16/emblems/emblem-annotations.png \
	icons/hicolor/32x32/actions/unannotate-symbolic.png \
	icons/hicolor/32x32/actions/annotate-symbolic.png \
	icons/hicolor/32x32/actions/annotate.png \
	icons/hicolor/32x32/actions/unannotate.png \
	icons/hicolor/32x32/emblems/emblem-annotations.png \
	icons/hicolor/512x512/actions/unannotate-symbolic.png \
	icons/hicolor/512x512/actions/annotate-symbolic.png \
	icons/hicolor/512x512/actions/annotate.png \
	icons/hicolor/512x512/actions/unannotate.png \
	icons/hicolor/512x512/emblems/emblem-annotations.png

EXTRA_DIST = \
	ChangeLog.md \
	INSTALL \
	NEWS \
	README.md \
	bootstrap \
	package.json \
	.editorconfig


if NLS_ENABLED

SUBDIRS += \
	po

else !NLS_ENABLED

EXTRA_DIST += \
	po

endif !NLS_ENABLED


BUILT_SOURCES = \
	no-dist/style.css

_clean_dirs_ = \
	no-dist

# Shell expansion is supported here
_distclean_dirs_ = \
	'po/backups'

# Shell expansion is supported here
_bootstrapclean_dirs_ = \
	'autom4te.cache' \
	'build-aux' \
	'm4'

# Shell expansion is supported here
_bootstrapclean_files_ = \
	*'~' \
	'ABOUT-NLS' \
	'aclocal.m4' \
	'compile' \
	'config'.* \
	'configure' \
	'depcomp' \
	'install-sh' \
	'libtool' \
	'ltmain.sh' \
	'missing' \
	'po/boldquot.sed' \
	'po/en@boldquot.header' \
	'po/en@quot.header' \
	'po/insert-header.sin' \
	'po'/*'~' \
	'po/Makefile.in.in' \
	'po/Makevars.template' \
	'po/quot.sed' \
	'po/remove-potcdate.sin' \
	'po/Rules-quot'


if HAVE_FIND

_bootstrapclean_dirs_ += \
	`$(FIND) -L . -type d -name .deps`


_bootstrapclean_files_ += \
	`$(FIND) -L . -type f -name 'Makefile.in'`

endif


no-dist/style.css: $(CSS_THEME)
	(test -d no-dist || (rm -f no-dist && $(MKDIR_P) no-dist)) && \
	cp '$(CSS_THEME)' '$@';


# Remove `$(docdir)` and `$(pkgdatadir)` on uninstall
uninstall-local:
	-rm -r '$(DESTDIR)$(docdir)' '$(DESTDIR)$(pkgdatadir)';


clean-local:
	-rm -rf $(_clean_dirs_);


distclean-local:
	-rm -rf $(_distclean_dirs_);


# Update translations
.PHONY: i18n-update
i18n-update:
	$(MKDIR_P) 'po/backups' && $(GREP) -o '^\s*\w\S\+' 'po/LINGUAS' | \
	while read _i_; do cp "po/$${_i_}.po" "po/backups/$${_i_}.po.bak" && \
	$(MAKE) -C 'po' "$${_i_}.po" && sed -i \
	's/"Project-Id-Version:.*$$/"Project-Id-Version: @PACKAGE_VERSION@\\n"/' \
	"po/$${_i_}.po"; done;


# Make the source directory depend on Autotools and a `bootstrap` script
.PHONY: bootstrap-clean
bootstrap-clean: maintainer-clean
	-rm -rf $(_bootstrapclean_dirs_); rm -f $(_bootstrapclean_files_); \
	(cd '$(srcdir)' && rm -rf $(_bootstrapclean_dirs_); \
	rm -f $(_bootstrapclean_files_));


# EOF

