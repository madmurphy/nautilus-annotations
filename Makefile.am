# Process this file with automake to produce Makefile.in


ACLOCAL_AMFLAGS = -I m4

SUBDIRS = \
	src

dist_doc_DATA = \
	AUTHORS \
	COPYING \
	ChangeLog \
	NEWS \
	README \
	docs/styling.md

dist_pkgdata_DATA = \
	extension-data/style.css

svgactionsdir = $(datadir)/icons/hicolor/scalable/actions
dist_svgactions_DATA = \
	extension-data/annotate.svg \
	extension-data/unannotate.svg

svgemblemsdir = $(datadir)/icons/hicolor/scalable/emblems
dist_svgemblems_DATA = \
	extension-data/emblem-annotations.svg

INTLTOOL_FILES = \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in

EXTRA_DIST = \
	ChangeLog.md \
	INSTALL \
	NEWS \
	README.md \
	bootstrap \
	package.json \
	.editorconfig \
	$(INTLTOOL_FILES)

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update \
	po/.intltool-merge-cache


# Shell expansion is supported here
_distclean_dirs_ = \
	'po/backups'


if NLS_ENABLED

SUBDIRS += \
	po

else !NLS_ENABLED

EXTRA_DIST += \
	po

endif !NLS_ENABLED


# Remove doc directory on uninstall
uninstall-local:
	-rm -r '$(DESTDIR)$(docdir)'


distclean-local:
	-rm -rf $(_distclean_dirs_);


# Update translations
.PHONY: i18n-update
i18n-update:
	(cd 'po' && intltool-update -p --gettext-package='$(PACKAGE_TARNAME)' \
	&& mkdir -p 'backups' && grep -o '^\s*\w\S\+' 'LINGUAS' | while read \
	coin; do mv "$${coin}.po" "$${coin}.po.bak" && msgmerge \
	"$${coin}.po.bak" '$(PACKAGE_TARNAME).pot' > "$${coin}.po" && mv \
	"$${coin}.po.bak" 'backups' && sed -i \
	's/"Project-Id-Version:\s*\([0-9]\+\(\.[0-9]\+\(\.[0-9]\)\?\)\?\)\?\+\\n"/"Project-Id-Version: @PACKAGE_VERSION@\\n"/' \
	"$${coin}.po"; done; rm '$(PACKAGE_TARNAME).pot')


# EOF

