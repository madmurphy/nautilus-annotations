/*  -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */

/*\
|*|
|*| nautilus-annotations.c
|*|
|*| Copyright (C) 2021 <madmurphy333@gmail.com>
|*|
|*| **Nautilus Annotations** is free software: you can redistribute it and/or
|*| modify it under the terms of the GNU General Public License as published by
|*| the Free Software Foundation, either version 3 of the License, or (at your
|*| option) any later version.
|*|
|*| **Nautilus Annotations** is distributed in the hope that it will be useful,
|*| but WITHOUT ANY WARRANTY; without even the implied warranty of
|*| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
|*| Public License for more details.
|*|
|*| You should have received a copy of the GNU General Public License along
|*| with this program. If not, see <http://www.gnu.org/licenses/>.
|*|
\*/



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <nautilus-extension.h>
#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>

#ifdef ENABLE_NLS
#include <libintl.h>
#include <glib/gi18n-lib.h>
#define I18N_INIT() \
	bindtextdomain(GETTEXT_PACKAGE, NAUTILUS_ANNOTATIONS_LOCALEDIR)
#else
#define _(STRING) ((char *) (STRING))
#define g_dngettext(DOMAIN, STRING1, STRING2, NUM) \
	((NUM) > 1 ? (char *) (STRING2) : (char *) (STRING1))
#define I18N_INIT()
#endif



/*\
|*|
|*|	GLOBAL TYPES AND VARIABLES
|*|
\*/


typedef struct {
	GObject parent_slot;
} NautilusAnnotations;

typedef struct {
	GObjectClass parent_slot;
} NautilusAnnotationsClass;

typedef struct {
	GtkWindow * main_window;
	GtkDialog * annotation_dialog;
	GtkSourceBuffer * source_buffer;
	GtkButton * discard_button;
	GList * file_selection;
} NautilusAnnotationsCommon;

typedef void (* OneParamFunc) (
	gpointer user_data
);

static GType provider_types[1];
static GType nautilus_annotations_type;
static GObjectClass * parent_class;

/*
Many other possibly key names would do, although `"metadata::annotation"` is
the key that was originally used by Nautilus.
*/
#ifndef G_FILE_ATTRIBUTE_METADATA_ANNOTATION
#define G_FILE_ATTRIBUTE_METADATA_ANNOTATION "metadata::annotation"
#endif



/*\
|*|
|*|	FUNCTIONS
|*|
\*/


static void destroy_na_common (gpointer const user_data) {

	#define na_common ((NautilusAnnotationsCommon *) user_data)

	if (na_common->annotation_dialog) {

		gtk_widget_destroy(GTK_WIDGET(na_common->annotation_dialog));

	}

	nautilus_file_info_list_free(na_common->file_selection);
	free(user_data);

	#undef na_common

}


static void quick_ok_cancel (
	GCallback const cb_ok,
	GCallback const cb_cancel,
	const char * const text_message,
	GtkWindow * const parent,
	gpointer const user_data
) {

	GtkWidget
		* const dialog =
			gtk_dialog_new_with_buttons(
				_("Annotations"),
				parent,
				GTK_DIALOG_MODAL,
				_("_OK"),
				GTK_RESPONSE_OK,
				_("_Cancel"),
				GTK_RESPONSE_CANCEL,
				NULL
			),
		* const label =
			gtk_label_new(text_message);

	#define NA_QUESTION_MARGINS 16
	gtk_widget_set_margin_top(label, NA_QUESTION_MARGINS);
	gtk_widget_set_margin_bottom(label, NA_QUESTION_MARGINS);
	gtk_widget_set_margin_start(label, NA_QUESTION_MARGINS);
	gtk_widget_set_margin_end(label, NA_QUESTION_MARGINS);
	#undef NA_QUESTION_MARGINS

	gtk_container_add(
		GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		label
	);

	gtk_widget_show(label);

	int response_id = gtk_dialog_run(GTK_DIALOG(dialog));

	if (cb_ok && response_id == GTK_RESPONSE_OK) {

		((OneParamFunc) cb_ok)(user_data);

	} else if (cb_cancel && response_id != GTK_RESPONSE_OK) {

		((OneParamFunc) cb_cancel)(user_data);

	}

	gtk_widget_destroy(dialog);

}


static void user_has_changed_text (
	GtkSourceBuffer * const text_buffer,
	gpointer const user_data
) {

	#define na_common ((NautilusAnnotationsCommon *) user_data)

	switch (
		(gtk_text_buffer_get_modified(GTK_TEXT_BUFFER(na_common->source_buffer)) << 1) |
		!na_common->discard_button
	) {

		case 0:

			gtk_widget_destroy(GTK_WIDGET(na_common->discard_button));
			na_common->discard_button = NULL;
			break;

		case 3:

			na_common->discard_button = GTK_BUTTON(
				gtk_dialog_add_button(
					na_common->annotation_dialog,
					_("_Discard changes"),
					GTK_RESPONSE_REJECT
				)
			);

	}

	#undef na_common

}


static void erase_annotations (
	gpointer const user_data
) {

	GError * set_err = NULL;

	for (
		GList * iter = ((NautilusAnnotationsCommon *) user_data)->file_selection;
			iter;
		iter = iter->next
	) {

		if (!g_file_set_attribute(
			nautilus_file_info_get_location(NAUTILUS_FILE_INFO(iter->data)),
			G_FILE_ATTRIBUTE_METADATA_ANNOTATION,
			G_FILE_ATTRIBUTE_TYPE_INVALID,
			NULL,
			G_FILE_QUERY_INFO_NONE,
			NULL,
			&set_err
		)) {

			fprintf(
				stderr,
				"Nautilus Annotations: %s - %s\n",
				set_err->message,
				_("Could not erase file's annotations")
			);
			g_error_free(set_err);
			set_err = NULL;
			continue;

		}

		nautilus_file_info_invalidate_extension_info(
			NAUTILUS_FILE_INFO(iter->data)
		);

	}

	destroy_na_common(user_data);

}

static void on_annotation_dialog_response (
	GtkDialog * const dialog,
	gint const response_id,
	gpointer const user_data
) {

	#define na_common ((NautilusAnnotationsCommon *) user_data)

	if (!gtk_text_buffer_get_modified(GTK_TEXT_BUFFER(na_common->source_buffer))) {

		goto destroy_and_leave;

	}

	GtkTextIter text_start, text_end;
	gchar * text_content;
	GError * set_err = NULL;

	switch (response_id) {

		case GTK_RESPONSE_DELETE_EVENT:

			gtk_text_buffer_get_bounds(
				GTK_TEXT_BUFFER(na_common->source_buffer),
				&text_start,
				&text_end
			);

			text_content = gtk_text_buffer_get_text(
				GTK_TEXT_BUFFER(na_common->source_buffer),
				&text_start,
				&text_end,
				false
			);

			if (!*text_content) {

				g_free(text_content);
				erase_annotations(user_data);
				return;

			}

			for (
				GList * iter = na_common->file_selection;
					iter;
				iter = iter->next
			) {

				if (
					!g_file_set_attribute_string(
						nautilus_file_info_get_location(NAUTILUS_FILE_INFO(iter->data)),
						G_FILE_ATTRIBUTE_METADATA_ANNOTATION,
						text_content,
						G_FILE_QUERY_INFO_NONE,
						NULL,
						&set_err
					)
				) {

					fprintf(
						stderr,
						"Nautilus Annotations: %s - %s\n",
						set_err->message,
						_("Could not save file's annotations")
					);
					g_error_free(set_err);
					set_err = NULL;
					continue;

				}

				nautilus_file_info_invalidate_extension_info(
					NAUTILUS_FILE_INFO(iter->data)
				);

			}

			g_free(text_content);
			break;

		case GTK_RESPONSE_REJECT:

			quick_ok_cancel(
				G_CALLBACK(destroy_na_common),
				NULL,
				_("Are you sure you want to discard the current changes?"),
				GTK_WINDOW(dialog),
				user_data
			);

			return;

	}


	/* \                                /\
	\ */     destroy_and_leave:        /* \
	 \/     ______________________     \ */


	destroy_na_common(na_common);

	#undef na_common

}


static void show_annotations (
	gpointer const user_data
) {

	#define na_common ((NautilusAnnotationsCommon *) user_data)

	GtkWidget
		* const scrollable =
			gtk_scrolled_window_new(NULL, NULL),
		* const text_view =
			gtk_source_view_new_with_buffer(na_common->source_buffer);

	na_common->annotation_dialog = GTK_DIALOG(gtk_dialog_new());

	gtk_window_set_modal(
		GTK_WINDOW(na_common->annotation_dialog),
		true
	);

	gchar
		* specific_title = NULL,
		* generic_title = _("Annotations");

	if (na_common->file_selection) {

		if (na_common->file_selection->next) {

			generic_title = _("Annotations shared between multiple files");

		} else {

			char * fpath = g_file_get_path(
				nautilus_file_info_get_location(
					NAUTILUS_FILE_INFO(na_common->file_selection->data)
				)
			);

			specific_title = g_strconcat(
				fpath ? fpath : "???",
				"\342\200\202" "\342\200\223" "\342\200\202",
				generic_title,
				NULL
			);

			g_free(fpath);

		}

	}

	gtk_window_set_title(
		GTK_WINDOW(na_common->annotation_dialog),
		specific_title ? specific_title : generic_title
	);

	g_free(specific_title);

	GdkRectangle workarea = { 0 };

	gdk_monitor_get_workarea(
		gdk_display_get_monitor_at_window(
			gdk_display_get_default(),
			gtk_widget_get_window(GTK_WIDGET(na_common->main_window))
		),
		&workarea
	);

	gtk_window_set_default_size(
		GTK_WINDOW(na_common->annotation_dialog),
		workarea.width ? workarea.width / 2 : 300,
		workarea.height ? workarea.height * 2 / 3 : 400
	);

	gtk_widget_set_vexpand(text_view, true);
	gtk_widget_set_hexpand(text_view, true);
	gtk_container_add(GTK_CONTAINER(scrollable), text_view);

	gtk_container_add(
		GTK_CONTAINER(gtk_dialog_get_content_area(na_common->annotation_dialog)),
		scrollable
	);

	g_signal_connect(
		na_common->annotation_dialog,
		"response",
		G_CALLBACK(on_annotation_dialog_response),
		na_common
	);

	g_signal_connect(
		na_common->source_buffer,
		"modified-changed",
		G_CALLBACK(user_has_changed_text),
		na_common
	);

	gtk_widget_show_all(GTK_WIDGET(na_common->annotation_dialog));

	#undef na_common

}

NautilusAnnotationsCommon * na_common_allocate_blank (
	GtkWindow * const window,
	GList * const file_selection,
	GtkSourceBuffer * const source_buffer
) {

	NautilusAnnotationsCommon * const
		na_common = malloc(sizeof(NautilusAnnotationsCommon));

	if (na_common) {

		*na_common = (NautilusAnnotationsCommon) {
			.main_window = window,
			.annotation_dialog = NULL,
			.source_buffer = source_buffer,
			.discard_button = NULL,
			.file_selection = file_selection
		};

	}

	return na_common;

}


static void nautilus_annotations_unannotate (
	NautilusMenuItem * const menu_item,
	gpointer const user_data
) {

	GList * const file_selection = g_object_get_data(
		(GObject *) menu_item,
		"nautilus_annotations_files"
	);

	if (!file_selection) {

		fprintf(
			stderr,
			"Nautilus Annotations: %s\n",
			_("No files were selected to be unannotated")
		);
		return;

	}

	NautilusAnnotationsCommon * const na_common = na_common_allocate_blank(
		GTK_WINDOW(user_data),
		nautilus_file_info_list_copy(file_selection),
		NULL
	);

	if (!na_common) {

		fprintf(
			stderr,
			"Nautilus Annotations: %s (ENOMEM ID: fx9dt6a)\n",
			_("Error allocating memory")
		);
		return;

	}

	quick_ok_cancel(
		G_CALLBACK(erase_annotations),
		G_CALLBACK(destroy_na_common),
		g_dngettext(
			GETTEXT_PACKAGE,
			"Do you really want to remove the annotations from the selected file?",
			"Do you really want to remove all annotations from the selected files?",
			g_list_length(file_selection)
		),
		GTK_WINDOW(user_data),
		na_common
	);

}


static void nautilus_annotations_annotate (
	NautilusMenuItem * const menu_item,
	gpointer const user_data
) {

	GList * const file_selection = g_object_get_data(
		(GObject *) menu_item,
		"nautilus_annotations_files"
	);

	if (!file_selection) {

		fprintf(
			stderr,
			"Nautilus Annotations: %s\n",
			_("No files were selected to be annotated")
		);
		return;

	}

	GFileInfo * finfo;
	const char * annotation_probe;
	GError * get_err = NULL;
	gchar * current_annotation = NULL;

	NautilusAnnotationsCommon * const na_common = na_common_allocate_blank(
		GTK_WINDOW(user_data),
		nautilus_file_info_list_copy(file_selection),
		gtk_source_buffer_new_with_language(
			gtk_source_language_manager_get_language(
				gtk_source_language_manager_get_default(),
				"markdown"
			)
		)
	);

	if (!na_common) {

		fprintf(
			stderr,
			"Nautilus Annotations: %s (ENOMEM ID: ym75llb)\n",
			_("Error allocating memory")
		);
		return;

	}

	for (GList * iter = file_selection; iter; iter = iter->next) {

		finfo = g_file_query_info(
			nautilus_file_info_get_location(NAUTILUS_FILE_INFO(iter->data)),
			G_FILE_ATTRIBUTE_METADATA_ANNOTATION,
			G_FILE_QUERY_INFO_NONE,
			NULL,
			&get_err
		);

		if (!finfo) {

			fprintf(
				stderr,
				"Nautilus Annotations: %s - %s\n",
				_("Could not access file's annotations"),
				get_err->message
			);
			g_error_free(get_err);
			get_err = NULL;
			return;

		}

		annotation_probe = g_file_info_get_attribute_string(
			finfo,
			G_FILE_ATTRIBUTE_METADATA_ANNOTATION
		);

		if (!annotation_probe) {

			goto unref_and_continue;

		}

		if (!current_annotation) {

			current_annotation = g_strdup(annotation_probe);
			goto unref_and_continue;

		}

		if (strcmp(annotation_probe, current_annotation)) {

			g_free(current_annotation);
			g_object_unref(finfo);
			quick_ok_cancel(
				G_CALLBACK(show_annotations),
				G_CALLBACK(destroy_na_common),
				_(
					"At least two annotations in the file selection differ.\n"
					"This will set up a blank new note."
				),
				GTK_WINDOW(user_data),
				na_common
			);
			return;

		}


		/* \                                /\
		\ */     unref_and_continue:       /* \
		 \/     ______________________     \ */


		g_object_unref(finfo);

	}

	if (current_annotation) {

		gtk_source_buffer_begin_not_undoable_action(na_common->source_buffer);

		gtk_text_buffer_set_text(
			GTK_TEXT_BUFFER(na_common->source_buffer),
			current_annotation,
			strlen(current_annotation)
		);

		gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(na_common->source_buffer), false);
		gtk_source_buffer_end_not_undoable_action(na_common->source_buffer);
		g_free(current_annotation);

	}

	show_annotations(na_common);

}


GType nautilus_annotations_get_type (void) {

	return nautilus_annotations_type;

}


static void nautilus_annotations_class_init (
	NautilusAnnotationsClass * const nautilus_annotations_class,
	gpointer const class_data
) {

	parent_class = g_type_class_peek_parent(nautilus_annotations_class);

}


static GList * nautilus_annotations_get_file_items (
	NautilusMenuProvider * const provider,
	GtkWidget * const window,
	GList * const file_selection
) {

	GFileInfo * finfo;
	GList * menu_items = NULL;
	NautilusMenuItem * menu_item = NULL;
	const guint sellen = g_list_length(file_selection);

	for (GList * iter = file_selection; iter; iter = iter->next) {

		finfo = g_file_query_info(
			nautilus_file_info_get_location(NAUTILUS_FILE_INFO(iter->data)),
			G_FILE_ATTRIBUTE_METADATA_ANNOTATION,
			G_FILE_QUERY_INFO_NONE,
			NULL,
			NULL
		);

		if (!finfo) {

			/*  Cannot get file's annotations  */
			continue;

		}

		if (
			g_file_info_get_attribute_string(
				finfo,
				G_FILE_ATTRIBUTE_METADATA_ANNOTATION
			)
		) {

			menu_item = nautilus_menu_item_new(
				"NautilusAnnotations::unannotate",
				g_dngettext(
					GETTEXT_PACKAGE,
					"_Remove annotations from file",
					"_Remove annotations from files",
					sellen
				),
				g_dngettext(
					GETTEXT_PACKAGE,
					"Remove annotations from the selected file",
					"Remove annotations from the selected files",
					sellen
				),
				"unannotate"
			);

			g_signal_connect(
				menu_item,
				"activate",
				G_CALLBACK(nautilus_annotations_unannotate),
				window
			);

			g_object_set_data_full(
				(GObject *) menu_item,
				"nautilus_annotations_files",
				nautilus_file_info_list_copy(file_selection),
				(GDestroyNotify) nautilus_file_info_list_free
			);

			menu_items = g_list_append(NULL, menu_item);
			g_object_unref(finfo);
			break;

		}

		g_object_unref(finfo);

	}

	if (menu_item) {

		menu_item = nautilus_menu_item_new(
			"NautilusAnnotations::annotate",
			g_dngettext(
				GETTEXT_PACKAGE,
				"Edit file's an_notations",
				"Edit files' an_notations",
				sellen
			),
			g_dngettext(
				GETTEXT_PACKAGE,
				"Edit the current annotations for this file",
				"Edit the current annotations for these files",
				sellen
			),
			"annotate"
		);

	} else {

		menu_item = nautilus_menu_item_new(
			"NautilusAnnotations::annotate",
			g_dngettext(
				GETTEXT_PACKAGE,
				"An_notate file",
				"An_notate files",
				sellen
			),
			g_dngettext(
				GETTEXT_PACKAGE,
				"Attach a note to this file",
				"Attach a note to these files",
				sellen
			),
			"annotate"
		);

	}

	g_signal_connect(
		menu_item,
		"activate",
		G_CALLBACK(nautilus_annotations_annotate),
		window
	);

	g_object_set_data_full(
		(GObject *) menu_item,
		"nautilus_annotations_files",
		nautilus_file_info_list_copy(file_selection),
		(GDestroyNotify) nautilus_file_info_list_free
	);

	return g_list_prepend(menu_items, menu_item);

}


NautilusOperationResult nautilus_annotations_update_file_info (
	NautilusInfoProvider * const provider,
	NautilusFileInfo * const file,
	GClosure * const update_complete,
	NautilusOperationHandle ** const handle
) {

	GFileInfo * const finfo = g_file_query_info(
		nautilus_file_info_get_location(file),
		G_FILE_ATTRIBUTE_METADATA_ANNOTATION,
		G_FILE_QUERY_INFO_NONE,
		NULL,
		NULL
	);

	if (!finfo) {

		return NAUTILUS_OPERATION_FAILED;

	}

	if (
		g_file_info_get_attribute_string(
			finfo,
			G_FILE_ATTRIBUTE_METADATA_ANNOTATION
		)
	) {

		nautilus_file_info_add_emblem(file, "emblem-annotations");

	}

	g_object_unref(finfo);
	return NAUTILUS_OPERATION_COMPLETE;

}


static void nautilus_annotations_menu_provider_iface_init (
	NautilusMenuProviderIface * const iface,
	gpointer const iface_data
) {

	iface->get_file_items = nautilus_annotations_get_file_items;

}


static void nautilus_annotations_type_info_provider_iface_init (
	NautilusInfoProviderIface * const iface,
	gpointer const iface_data
) {

	iface->update_file_info = nautilus_annotations_update_file_info;

}


static void nautilus_annotations_register_type (
	GTypeModule * const module
) {

	static const GTypeInfo info = {
		sizeof(NautilusAnnotationsClass),
		(GBaseInitFunc) NULL,
		(GBaseFinalizeFunc) NULL,
		(GClassInitFunc) nautilus_annotations_class_init,
		(GClassFinalizeFunc) NULL,
		NULL,
		sizeof(NautilusAnnotations),
		0,
		(GInstanceInitFunc) NULL,
		(GTypeValueTable * ) NULL
	};

	nautilus_annotations_type = g_type_module_register_type(
		module,
		G_TYPE_OBJECT,
		"NautilusAnnotations",
		&info,
		0
	);

	static const GInterfaceInfo type_info_provider_iface_info = {
		(GInterfaceInitFunc) nautilus_annotations_type_info_provider_iface_init,
		(GInterfaceFinalizeFunc) NULL,
		NULL
	};

	g_type_module_add_interface(
		module,
		nautilus_annotations_type,
		NAUTILUS_TYPE_INFO_PROVIDER,
		&type_info_provider_iface_info
	);

	static const GInterfaceInfo menu_provider_iface_info = {
		(GInterfaceInitFunc) nautilus_annotations_menu_provider_iface_init,
		(GInterfaceFinalizeFunc) NULL,
		NULL
	};

	g_type_module_add_interface(
		module,
		nautilus_annotations_type,
		NAUTILUS_TYPE_MENU_PROVIDER,
		&menu_provider_iface_info
	);

}


void nautilus_module_initialize (
	GTypeModule * const module
) {

	I18N_INIT();
	nautilus_annotations_register_type(module);
	*provider_types = nautilus_annotations_get_type();

}


void nautilus_module_shutdown (void) {

	/*  Any module-specific shutdown  */

}


void nautilus_module_list_types (
	const GType ** types,
	int * num_types
) {

	*types = provider_types;
	*num_types = G_N_ELEMENTS(provider_types);

}


/*  EOF  */

