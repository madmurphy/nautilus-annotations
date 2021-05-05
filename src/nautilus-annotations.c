/*  -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*  Please make sure that the TAB width in your editor is set to 4 spaces  */

/*\
|*|
|*| nautilus-annotations.c
|*|
|*| https://gitlab.gnome.org/madmurphy/nautilus-annotations
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
#include <string.h>
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

static GType provider_types[1];
static GType nautilus_annotations_type;
static GObjectClass * parent_class;
static GtkCssProvider * annotations_css;

/*  The metadata key that was originally used by Nautilus  */
#ifndef G_FILE_ATTRIBUTE_METADATA_ANNOTATION
#define G_FILE_ATTRIBUTE_METADATA_ANNOTATION "metadata::annotation"
#endif

/*  The CSS to add to the screen  */
#define NAUTILUS_ANNOTATIONS_CSS PACKAGE_DATA_DIR "/style.css"



/*\
|*|
|*|	FUNCTIONS
|*|
\*/


static void destroy_na_common (gpointer const v_na_common) {

	#define na_common ((NautilusAnnotationsCommon *) v_na_common)

	if (na_common->annotation_dialog) {

		gtk_widget_destroy(GTK_WIDGET(na_common->annotation_dialog));

	}

	nautilus_file_info_list_free(na_common->file_selection);
	free(v_na_common);

	#undef na_common

}


static bool destructive_action_confirm (
	GtkWindow * const parent,
	const char * const primary_text,
	const char * const secondary_text,
	const char * const destructive_text,
	GCallback const cb_ok,
	GCallback const cb_cancel,
	gpointer const data
) {

	GtkDialog * const question_dialog =
		GTK_DIALOG(
			gtk_message_dialog_new(
				parent,
				0,
				GTK_MESSAGE_QUESTION,
				GTK_BUTTONS_CANCEL,
				NULL
			)
		);

	GtkWidget * const destructive_button =
		gtk_dialog_add_button(
			question_dialog,
			destructive_text,
			GTK_RESPONSE_OK
		);

	g_object_set(
		G_OBJECT(question_dialog),
		"text",
		primary_text,
		"secondary-text",
		secondary_text,
		NULL
	);

	gtk_dialog_set_default_response(
		question_dialog,
		GTK_RESPONSE_OK
	);

	gtk_style_context_add_class(
		gtk_widget_get_style_context(destructive_button),
		"destructive-action"
	);

	guint8 response_state =
		gtk_dialog_run(question_dialog) == GTK_RESPONSE_OK ?
			1 | (!cb_ok << 1)
		:
			0 | (!cb_cancel << 1);

	gtk_widget_destroy(GTK_WIDGET(question_dialog));

	switch (response_state) {

		/*

		`case 0` ==> User chose "Cancel" and callback is available
		`case 1` ==> User chose "OK" and callback is available
		`case 2` ==> User chose "Cancel", but no callback is available
		`case 3` ==> User chose "OK", but no callback is available

		*/

		case 1: ((void (*) (gpointer)) cb_ok)(data); case 3:

			return true;

		case 0: ((void (*) (gpointer)) cb_cancel)(data); default:

			return false;

	}

}


static void on_text_modified_state_change (
	GtkSourceBuffer * const text_buffer,
	gpointer const v_na_common
) {

	#define na_common ((NautilusAnnotationsCommon *) v_na_common)

	switch (
		(
			gtk_text_buffer_get_modified(
				GTK_TEXT_BUFFER(na_common->source_buffer)
			) << 1
		) |
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

			gtk_style_context_add_class(
				gtk_widget_get_style_context(
					GTK_WIDGET(na_common->discard_button)
				),
				"nautilus-discard-annotations"
			);

		/*

		`case 1` ==> The `"modified"` bit flipped to `false` but for obscure
			reasons the discard button was already gone
		`case 2` ==> The `"modified"` bit flipped to `true` but for obscure
			reasons the discard button was already there

		*/

	}

	#undef na_common

}


static void erase_annotations (
	gpointer const v_na_common
) {

	#define na_common ((NautilusAnnotationsCommon *) v_na_common)

	GError * set_err = NULL;

	for (GList * iter = na_common->file_selection; iter; iter = iter->next) {

		if (
			!g_file_set_attribute(
				nautilus_file_info_get_location(
					NAUTILUS_FILE_INFO(iter->data)
				),
				G_FILE_ATTRIBUTE_METADATA_ANNOTATION,
				G_FILE_ATTRIBUTE_TYPE_INVALID,
				NULL,
				G_FILE_QUERY_INFO_NONE,
				NULL,
				&set_err
			)
		) {

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

	destroy_na_common(v_na_common);

	#undef na_common

}


static void on_annotation_dialog_response (
	GtkDialog * const dialog,
	gint const response_id,
	gpointer const v_na_common
) {

	#define na_common ((NautilusAnnotationsCommon *) v_na_common)

	if (
		!gtk_text_buffer_get_modified(
			GTK_TEXT_BUFFER(na_common->source_buffer)
		)
	) {

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
				erase_annotations(v_na_common);
				return;

			}

			for (
				GList * iter = na_common->file_selection;
					iter;
				iter = iter->next
			) {

				if (
					!g_file_set_attribute_string(
						nautilus_file_info_get_location(
							NAUTILUS_FILE_INFO(iter->data)
						),
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

			if (
				!destructive_action_confirm(
					GTK_WINDOW(dialog),
					_("Are you sure you want to discard the current changes?"),
					_("This action cannot be undone."),
					_("_Discard changes"),
					NULL,
					NULL,
					NULL
				)
			) {

				return;

			}

	}


	/* \                                /\
	\ */     destroy_and_leave:        /* \
	 \/     ______________________     \ */


	destroy_na_common(na_common);

	#undef na_common

}


static void show_annotations (
	gpointer const v_na_common
) {

	#define na_common ((NautilusAnnotationsCommon *) v_na_common)

	GtkWidget
		* const scrollable =
			gtk_scrolled_window_new(NULL, NULL),
		* const text_area =
			gtk_source_view_new_with_buffer(na_common->source_buffer);

	na_common->annotation_dialog = GTK_DIALOG(gtk_dialog_new());

	gtk_style_context_add_class(
		gtk_widget_get_style_context(GTK_WIDGET(na_common->annotation_dialog)),
		"nautilus-annotations"
	);

	gtk_style_context_add_class(
		gtk_widget_get_style_context(text_area),
		"nautilus-textarea-annotations"
	);

	gtk_window_set_modal(
		GTK_WINDOW(na_common->annotation_dialog),
		true
	);

	gtk_window_set_transient_for(
		GTK_WINDOW(na_common->annotation_dialog),
		na_common->main_window
	);

	gchar
		* specific_title = NULL,
		* generic_title = _("Annotations");

	if (na_common->file_selection) {

		if (na_common->file_selection->next) {

			generic_title = _("Annotations shared between multiple files");

		} else {

			char * const fpath = g_file_get_path(
				nautilus_file_info_get_location(
					NAUTILUS_FILE_INFO(na_common->file_selection->data)
				)
			);

			gchar * shown_path;

			if ((shown_path = fpath)) {

				const gchar * const homedir = g_get_home_dir();

				if (homedir && *homedir && g_str_has_prefix(fpath, homedir)) {

					*(shown_path += strlen(homedir) - 1) = '~';

				}

			} else {

				shown_path = "???";

			}

			specific_title = g_strconcat(
				shown_path,
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

	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_area), GTK_WRAP_WORD);
	gtk_widget_set_vexpand(text_area, true);
	gtk_widget_set_hexpand(text_area, true);

	gtk_container_add(GTK_CONTAINER(scrollable), text_area);

	gtk_container_add(
		GTK_CONTAINER(
			gtk_dialog_get_content_area(na_common->annotation_dialog)
		),
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
		G_CALLBACK(on_text_modified_state_change),
		na_common
	);

	gtk_widget_show_all(GTK_WIDGET(na_common->annotation_dialog));

	#undef na_common

}


static NautilusAnnotationsCommon * na_common_allocate_blank (
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
	gpointer const v_window
) {

	GList * const file_selection = g_object_get_data(
		G_OBJECT(menu_item),
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

	NautilusAnnotationsCommon * const na_common =
		na_common_allocate_blank(
			GTK_WINDOW(v_window),
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

	destructive_action_confirm(
		GTK_WINDOW(v_window),
		_("Do you really want to erase the annotations attached?"),
		_("The annotations will be lost forever."),
		_("E_rase"),
		G_CALLBACK(erase_annotations),
		G_CALLBACK(destroy_na_common),
		na_common
	);

}


static void nautilus_annotations_annotate (
	NautilusMenuItem * const menu_item,
	gpointer const v_window
) {

	GList * const file_selection = g_object_get_data(
		G_OBJECT(menu_item),
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

	NautilusAnnotationsCommon * const na_common =
		na_common_allocate_blank(
			GTK_WINDOW(v_window),
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
			destructive_action_confirm(
				GTK_WINDOW(v_window),
				_("At least two annotations in the file selection differ"),
				_("This will set up a blank new annotation."),
				_("_OK"),
				G_CALLBACK(show_annotations),
				G_CALLBACK(destroy_na_common),
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

		gtk_text_buffer_set_modified(
			GTK_TEXT_BUFFER(na_common->source_buffer),
			false
		);

		gtk_source_buffer_end_not_undoable_action(na_common->source_buffer);
		g_free(current_annotation);

	}

	show_annotations(na_common);

}


static GList * nautilus_annotations_get_file_items (
	NautilusMenuProvider * const provider,
	GtkWidget * const window,
	GList * const file_selection
) {

	#define NA_IS_FILE_SELECTION 1
	#define NA_IS_DIRECTORY_SELECTION 2

	#define NA_HAVE_ANNOTATED 1
	#define NA_HAVE_UNANNOTATED 2

	guint8
		selection_type = 0,
		selection_content = 0;

	gsize sellen = 0;
	GList * iter;
	GFileInfo * finfo;

	for (iter = file_selection; iter; sellen++, iter = iter->next) {

		selection_type |= nautilus_file_info_is_directory(
			NAUTILUS_FILE_INFO(iter->data)
		) ? NA_IS_DIRECTORY_SELECTION : NA_IS_FILE_SELECTION;

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

		selection_content |= g_file_info_get_attribute_string(
				finfo,
				G_FILE_ATTRIBUTE_METADATA_ANNOTATION
			) ? NA_HAVE_ANNOTATED : NA_HAVE_UNANNOTATED;

		g_object_unref(finfo);

		if (
			!(~selection_content & (NA_HAVE_ANNOTATED | NA_HAVE_UNANNOTATED))
		) {

			break;

		}

	}

	while (iter) {

		selection_type |= nautilus_file_info_is_directory(
			NAUTILUS_FILE_INFO(iter->data)
		) ? NA_IS_DIRECTORY_SELECTION : NA_IS_FILE_SELECTION;

		sellen++;
		iter = iter->next;

	}

	NautilusMenuItem
		* item_annotations,
		* item_annotate;

	if (selection_content & NA_HAVE_ANNOTATED) {

		NautilusMenu * const menu_annotations = nautilus_menu_new();
		NautilusMenuItem * subitem_iter;

		item_annotations = nautilus_menu_item_new(
			"NautilusAnnotations::annotations",
			selection_type == NA_IS_DIRECTORY_SELECTION ?
				g_dngettext(
					GETTEXT_PACKAGE,
					"Directory's _annotations",
					"Directories' _annotations",
					sellen
				)
			: selection_type == NA_IS_FILE_SELECTION ?
				g_dngettext(
					GETTEXT_PACKAGE,
					"File's _annotations",
					"Files' _annotations",
					sellen
				)
			:
				_("Objects' _annotations"),
			g_dngettext(
				GETTEXT_PACKAGE,
				"Choose an action for the object's annotations",
				"Choose an action for the objects' annotations",
				sellen
			),
			"unannotate"
		);

		nautilus_menu_item_set_submenu(item_annotations, menu_annotations);

		item_annotate = nautilus_menu_item_new(
			"NautilusAnnotations::annotate",
			selection_content & NA_HAVE_UNANNOTATED ?
				_("_Edit and extend")
			:
				_("_Edit"),
			selection_content & NA_HAVE_UNANNOTATED ?
				_(
					"Edit and extend the annotations attached to the selected"
					" objects"
				)
			:
				g_dngettext(
					GETTEXT_PACKAGE,
					"Edit the annotations attached to the selected object",
					"Edit the annotations attached to the selected objects",
					sellen
				),
			"annotate"
		);

		nautilus_menu_append_item(menu_annotations, item_annotate);

		subitem_iter = nautilus_menu_item_new(
			"NautilusAnnotations::unannotate",
			_("E_rase"),
			g_dngettext(
				GETTEXT_PACKAGE,
				"Remove the annotations attached to the selected object",
				"Remove the annotations attached to the selected objects",
				sellen
			),
			"unannotate"
		);

		g_signal_connect(
			subitem_iter,
			"activate",
			G_CALLBACK(nautilus_annotations_unannotate),
			window
		);

		g_object_set_data_full(
			G_OBJECT(subitem_iter),
			"nautilus_annotations_files",
			nautilus_file_info_list_copy(file_selection),
			(GDestroyNotify) nautilus_file_info_list_free
		);

		nautilus_menu_append_item(menu_annotations, subitem_iter);

	} else {

		item_annotations = item_annotate = nautilus_menu_item_new(
			"NautilusAnnotations::annotate",
			selection_type == NA_IS_DIRECTORY_SELECTION ?
				g_dngettext(
					GETTEXT_PACKAGE,
					"_Annotate directory",
					"_Annotate directories",
					sellen
				)
			: selection_type == NA_IS_FILE_SELECTION ?
				g_dngettext(
					GETTEXT_PACKAGE,
					"_Annotate file",
					"_Annotate files",
					sellen
				)
			:
				_("_Annotate objects"),
			g_dngettext(
				GETTEXT_PACKAGE,
				"Attach an annotation to the selected object",
				"Attach an annotation to the selected objects",
				sellen
			),
			"annotate"
		);

	}

	g_signal_connect(
		item_annotate,
		"activate",
		G_CALLBACK(nautilus_annotations_annotate),
		window
	);

	g_object_set_data_full(
		G_OBJECT(item_annotate),
		"nautilus_annotations_files",
		nautilus_file_info_list_copy(file_selection),
		(GDestroyNotify) nautilus_file_info_list_free
	);

	return g_list_append(NULL, item_annotations);

	#undef NA_HAVE_UNANNOTATED
	#undef NA_HAVE_ANNOTATED

	#undef NA_IS_DIRECTORY_SELECTION
	#undef NA_IS_FILE_SELECTION

}


GList * nautilus_annotations_get_background_items(
	NautilusMenuProvider * provider,
	GtkWidget * window,
	NautilusFileInfo * current_folder
) {

	GList
		* file_selection = g_list_append(NULL, current_folder),
		* menu_items = nautilus_annotations_get_file_items(
			provider,
			window,
			file_selection
		);

	g_list_free(file_selection);

	return menu_items;

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

		/*  Cannot get file's annotations  */
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


static void nautilus_annotations_type_info_provider_iface_init (
	NautilusInfoProviderIface * const iface,
	gpointer const iface_data
) {

	iface->update_file_info = nautilus_annotations_update_file_info;

}


static void nautilus_annotations_menu_provider_iface_init (
	NautilusMenuProviderIface * const iface,
	gpointer const iface_data
) {

	iface->get_file_items = nautilus_annotations_get_file_items;
	iface->get_background_items = nautilus_annotations_get_background_items;

}


static void nautilus_annotations_class_init (
	NautilusAnnotationsClass * const nautilus_annotations_class,
	gpointer const class_data
) {

	parent_class = g_type_class_peek_parent(nautilus_annotations_class);

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
		(GInterfaceInitFunc)
			nautilus_annotations_type_info_provider_iface_init,
		(GInterfaceFinalizeFunc) NULL,
		NULL
	};

	static const GInterfaceInfo menu_provider_iface_info = {
		(GInterfaceInitFunc) nautilus_annotations_menu_provider_iface_init,
		(GInterfaceFinalizeFunc) NULL,
		NULL
	};

	g_type_module_add_interface(
		module,
		nautilus_annotations_type,
		NAUTILUS_TYPE_INFO_PROVIDER,
		&type_info_provider_iface_info
	);

	g_type_module_add_interface(
		module,
		nautilus_annotations_type,
		NAUTILUS_TYPE_MENU_PROVIDER,
		&menu_provider_iface_info
	);

}


GType nautilus_annotations_get_type (void) {

	return nautilus_annotations_type;

}


void nautilus_module_initialize (
	GTypeModule * const module
) {

	I18N_INIT();
	nautilus_annotations_register_type(module);
	*provider_types = nautilus_annotations_get_type();
	annotations_css = gtk_css_provider_new();

	gtk_css_provider_load_from_path(
		annotations_css,
		NAUTILUS_ANNOTATIONS_CSS,
		NULL
	);

	gtk_style_context_add_provider_for_screen(
		gdk_display_get_default_screen(gdk_display_get_default()),
		GTK_STYLE_PROVIDER(annotations_css),
		GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
	);

}


void nautilus_module_shutdown (void) {

	gtk_style_context_remove_provider_for_screen(
		gdk_display_get_default_screen(gdk_display_get_default()),
		GTK_STYLE_PROVIDER(annotations_css)
	);

	g_object_unref(annotations_css);

}


void nautilus_module_list_types (
	const GType ** const types,
	int * const num_types
) {

	*types = provider_types;
	*num_types = G_N_ELEMENTS(provider_types);

}


/*  EOF  */

