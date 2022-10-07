# Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	"-DPACKAGE_LOCALE_DIR=\"$(localedir)\"" \
	"-DPACKAGE_SRC_DIR=\"$(srcdir)\"" \
	"-DPACKAGE_DATA_DIR=\"$(pkgdatadir)\"" \
	"-DG_LOG_DOMAIN=\"NautilusAnnotations\""

AM_CFLAGS = \
	-Wall \
	-Wextra \
	-g \
	-pedantic \
	$(WARN_CFLAGS) \
	$(NAUTILUS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(NAUTILUS_ANNOTATIONS_CFLAGS)

nautilus_extensiondir = $(NAUTILUS_EXTENSION_DIR)

nautilus_extension_LTLIBRARIES = \
	libnautilus-annotations.la

libnautilus_annotations_la_SOURCES = \
	nautilus-annotations.c

libnautilus_annotations_la_LDFLAGS = \
	-module \
	-avoid-version

libnautilus_annotations_la_LIBADD = \
	$(NAUTILUS_ANNOTATIONS_LIBS)

# EOF

