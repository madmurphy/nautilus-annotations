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
	$(WARN_CFLAGS) \
	$(NAUTILUS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(NAUTILUS_ANNOTATIONS_CFLAGS)


BUILT_SOURCES = \
	nautilus-annotations.gresources.c \
	nautilus-annotations.gresources.h


CLEANFILES = \
	$(BUILT_SOURCES)


EXTRA_DIST = \
	emblem-picker \
	resources


nautilus_extensiondir = $(NAUTILUS_EXTENSION_DIR)


nautilus_extension_LTLIBRARIES = \
	libnautilus-annotations.la


libnautilus_annotations_la_SOURCES = \
	nautilus-annotations.gresources.c \
	nautilus-annotations.gresources.h \
	emblem-picker/gnui-definitions.h \
	emblem-picker/gnui-emblem-picker.c \
	emblem-picker/gnui-emblem-picker.h \
	emblem-picker/gnui-internals.h \
	nautilus-annotations.c


libnautilus_annotations_la_LDFLAGS = \
	-module \
	-avoid-version


libnautilus_annotations_la_LIBADD = \
	$(NAUTILUS_ANNOTATIONS_LIBS)


libnautilus_annotations_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	'-DGNUISANCE_CONST_BUILD_FLAG_CPP_UNIT=GNUI_EMBLEM_PICKER' \
	'-DGNUISANCE_CONST_BUILD_FLAG_C_UNIT=gnui_emblem_picker' \
	'-DGNUISANCE_CONST_BUILD_FLAG_NO_ENVIRONMENT'


nautilus-annotations.gresources.c: \
 resources/nautilus-annotations.gresources.xml
	glib-compile-resources --sourcedir='resources' --target='$@' \
	--c-name gnui_emblem_picker --generate-source '$<';


nautilus-annotations.gresources.h: \
 resources/nautilus-annotations.gresources.xml
	glib-compile-resources --sourcedir='resources' --target='$@' \
	--c-name gnui_emblem_picker --generate-header '$<';


# EOF

