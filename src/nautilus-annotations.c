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
#include "config.h"
#endif

#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>
#include <nautilus-extension.h>



/*\
|*|
|*| BUILD SETTINGS
|*|
\*/


#ifdef ENABLE_NLS
#include <glib/gi18n-lib.h>
#define I18N_INIT() \
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR)
#else
#define _(STRING) ((char *) (STRING))
#define g_dngettext(DOMAIN, STRING1, STRING2, NUM) \
	((NUM) > 1 ? (char *) (STRING2) : (char *) (STRING1))
#define I18N_INIT()
#endif



/*\
|*|
|*| GLOBAL TYPES AND VARIABLES
|*|
\*/


#define UNUSED(ARG) (void)(ARG)

typedef struct {
	GObject parent_slot;
} NautilusAnnotations;

typedef struct {
	GObjectClass parent_slot;
} NautilusAnnotationsClass;

typedef struct {
	GtkDialog * annotation_dialog;
	GtkSourceBuffer * annotation_text;
	GtkButton * discard_button;
	GList * targets;
} NautilusAnnotationsSession;

static GType provider_types[1];
static GType nautilus_annotations_type;
static GObjectClass * parent_class;
static GtkCssProvider * annotations_css;

/*  This metadata key was originally used by Nautilus  */
#ifndef G_FILE_ATTRIBUTE_METADATA_ANNOTATION
#define G_FILE_ATTRIBUTE_METADATA_ANNOTATION "metadata::annotation"
#endif

/*  Default size of the annotation window when no workarea is available  */
#define A8N_WIN_FALLBACK_WIDTH 300
#define A8N_WIN_FALLBACK_HEIGHT 400

/*  This constant is measured in number of unicode characters  */
#define A8N_COLUMN_MAX_LENGTH 99



/*\
|*|
|*| FUNCTIONS
|*|
\*/


static bool destructive_action_confirm (
	GtkWindow * const parent,
	const gchar * const primary_text,
	const gchar * const secondary_text,
	/*  nullable  */  const gchar * const destructive_text
) {

	GtkDialog * const question_dialog =
		GTK_DIALOG(
			gtk_message_dialog_new(
				parent,
				0,
				GTK_MESSAGE_QUESTION,
				destructive_text ? GTK_BUTTONS_CANCEL : GTK_BUTTONS_OK_CANCEL,
				"%s",
				primary_text
			)
		);

	gtk_message_dialog_format_secondary_text(
		GTK_MESSAGE_DIALOG(question_dialog),
		"%s",
		secondary_text
	);

	GtkWidget * const destructive_button =
		destructive_text ?
			gtk_dialog_add_button(
				question_dialog,
				destructive_text,
				GTK_RESPONSE_OK
			)
		:
			gtk_dialog_get_widget_for_response(
				question_dialog,
				GTK_RESPONSE_OK
			);

	gtk_dialog_set_default_response(question_dialog, GTK_RESPONSE_OK);

	gtk_style_context_add_class(
		gtk_widget_get_style_context(destructive_button),
		"destructive-action"
	);

	const bool b_confirmed =
		gtk_dialog_run(question_dialog) == GTK_RESPONSE_OK;

	gtk_widget_destroy(GTK_WIDGET(question_dialog));
	return b_confirmed;

}


static guint clear_files (
	GList * const file_selection
) {

	GFile * location;
	GError * eraserr = NULL;
	guint noncleared = 0;
	gchar * uri;

	for (GList * iter = file_selection; iter; iter = iter->next) {

		location = nautilus_file_info_get_location(
			NAUTILUS_FILE_INFO(iter->data)
		);

		if (
			g_file_set_attribute(
				location,
				G_FILE_ATTRIBUTE_METADATA_ANNOTATION,
				G_FILE_ATTRIBUTE_TYPE_INVALID,
				NULL,
				G_FILE_QUERY_INFO_NONE,
				NULL,
				&eraserr
			)
		) {

			nautilus_file_info_invalidate_extension_info(
				NAUTILUS_FILE_INFO(iter->data)
			);

		} else {

			uri = nautilus_file_info_get_uri(NAUTILUS_FILE_INFO(iter->data));

			g_message(
				"%s (%s) // %s",
				_("Could not erase file's annotations"),
				uri ? uri : _("unknown path"),
				eraserr->message
			);

			g_free(uri);
			g_clear_error(&eraserr);
			noncleared++;

		}

		g_object_unref(location);

	}

	return noncleared;

}


static void annotation_session_destroy (
	NautilusAnnotationsSession * const session
) {

	gtk_widget_destroy(GTK_WIDGET(session->annotation_dialog));
	nautilus_file_info_list_free(session->targets);
	g_free(session);

}


static guint annotation_session_export (
	NautilusAnnotationsSession * const session
) {

	GtkTextIter text_start, text_end;
	gchar * text_content;

	gtk_text_buffer_get_bounds(
		GTK_TEXT_BUFFER(session->annotation_text),
		&text_start,
		&text_end
	);

	text_content = gtk_text_buffer_get_text(
		GTK_TEXT_BUFFER(session->annotation_text),
		&text_start,
		&text_end,
		false
	);

	if (!*text_content) {

		g_free(text_content);
		return clear_files(session->targets);

	}

	guint unsaved = 0;
	GFile * location;
	GError * saverr = NULL;
	gchar * uri;

	for (GList * iter = session->targets; iter; iter = iter->next) {

		location = nautilus_file_info_get_location(
			NAUTILUS_FILE_INFO(iter->data)
		);

		if (
			g_file_set_attribute_string(
				location,
				G_FILE_ATTRIBUTE_METADATA_ANNOTATION,
				text_content,
				G_FILE_QUERY_INFO_NONE,
				NULL,
				&saverr
			)
		) {

			nautilus_file_info_invalidate_extension_info(
				NAUTILUS_FILE_INFO(iter->data)
			);

		} else {

			uri = nautilus_file_info_get_uri(NAUTILUS_FILE_INFO(iter->data));

			g_message(
				"%s (%s) // %s",
				_("Could not save file's annotations"),
				uri ? uri : _("unknown path"),
				saverr->message
			);

			g_free(uri);
			g_clear_error(&saverr);
			unsaved++;

		}

		g_object_unref(location);

	}

	g_free(text_content);
	return unsaved;

}


static void annotation_session_save (
	NautilusAnnotationsSession * const session
) {

	if (!annotation_session_export(session)) {

		gtk_text_buffer_set_modified(
			GTK_TEXT_BUFFER(session->annotation_text),
			false
		);

	}

}


static void annotation_session_exit (
	NautilusAnnotationsSession * const session
) {

	if (
		gtk_text_buffer_get_modified(
			GTK_TEXT_BUFFER(session->annotation_text)
		)
	) {

		annotation_session_export(session);

	}

	annotation_session_destroy(session);

}


static void annotation_session_discard (
	NautilusAnnotationsSession * const session
) {

	if (
		!gtk_text_buffer_get_modified(
			GTK_TEXT_BUFFER(session->annotation_text)
		) || destructive_action_confirm(
			GTK_WINDOW(session->annotation_dialog),
			_("Are you sure you want to discard the current changes?"),
			_("This action cannot be undone."),
			_("_Discard changes")
		)
	) {

		annotation_session_destroy(session);

	}

}


static void on_annotation_dialog_response (
	GtkDialog * const dialog,
	gint const response_id,
	NautilusAnnotationsSession * const session
) {

    UNUSED(dialog);

	switch (response_id) {

		case GTK_RESPONSE_REJECT:

			annotation_session_discard(session);
			return;

		default:

			annotation_session_exit(session);

	}

}


static void on_text_modified_state_change (
	GtkSourceBuffer * const annotation_text,
	NautilusAnnotationsSession * const session
) {

	gtk_widget_set_visible(
		GTK_WIDGET(session->discard_button),
		gtk_text_buffer_get_modified(GTK_TEXT_BUFFER(annotation_text))
	);

}


static void annotation_session_new_with_text (
	GtkWindow * const parent,
	GList * const target_files,
	/*  nullable  */  const gchar * const initial_text
) {

	NautilusAnnotationsSession
		* const session = g_new(NautilusAnnotationsSession, 1);

	GtkDialog * const a8n_dialog = GTK_DIALOG(
		g_object_new(GTK_TYPE_DIALOG, "use-header-bar", true, NULL)
	);

	*session = (NautilusAnnotationsSession) {
		.annotation_dialog = a8n_dialog,
		.annotation_text = gtk_source_buffer_new_with_language(
			gtk_source_language_manager_get_language(
				gtk_source_language_manager_get_default(),
				"markdown"
			)
		),
		.discard_button = GTK_BUTTON(
			gtk_dialog_add_button(
				a8n_dialog,
				_("_Discard changes"),
				GTK_RESPONSE_REJECT
			)
		),
		.targets = nautilus_file_info_list_copy(target_files)
	};

	gtk_window_set_modal(GTK_WINDOW(a8n_dialog), true);
	gtk_window_set_transient_for(GTK_WINDOW(a8n_dialog), parent);
	gtk_widget_hide(GTK_WIDGET(session->discard_button));
	gtk_widget_set_no_show_all(GTK_WIDGET(session->discard_button), true);

	if (initial_text) {

		gtk_source_buffer_begin_not_undoable_action(session->annotation_text);

		gtk_text_buffer_set_text(
			GTK_TEXT_BUFFER(session->annotation_text),
			initial_text,
			strlen(initial_text)
		);

		gtk_text_buffer_set_modified(
			GTK_TEXT_BUFFER(session->annotation_text),
			false
		);

		gtk_source_buffer_end_not_undoable_action(session->annotation_text);

	}

	gtk_style_context_add_class(
		gtk_widget_get_style_context(GTK_WIDGET(a8n_dialog)),
		"nautilus-annotations-dialog"
	);

	gtk_style_context_add_class(
		gtk_widget_get_style_context(GTK_WIDGET(session->discard_button)),
		"nautilus-annotations-discard"
	);

	gchar
		* header_subtitle,
		* tmpbuf = NULL;

	const gchar * header_title;

	if (session->targets->next) {

		header_title = _("Shared annotations");
		header_subtitle = _("(multiple objects)");

	} else {

		GFile * location = nautilus_file_info_get_location(
			NAUTILUS_FILE_INFO(session->targets->data)
		);

		tmpbuf = g_file_get_path(location);
		g_object_unref(location);
		header_title = _("Annotations");

		if (tmpbuf) {

			const gchar * const homedir = g_get_home_dir();

			header_subtitle = tmpbuf;

			if (homedir && *homedir && g_str_has_prefix(tmpbuf, homedir)) {

				/*  Current user (`~/doc.md`, `~/Videos`, etc.)  */
				*(header_subtitle += strlen(homedir) - 1) = '~';

			} else if (g_str_has_prefix(tmpbuf, "/home")) {

				/*  Others (`~john/doc.md`, `~lisa/Videos`, etc.)  */
				*(header_subtitle += sizeof("/home") - 1) = '~';

			} else if (g_str_has_prefix(tmpbuf, "/root")) {

				/*  Super user (`~root/doc.md`, `~root/Videos`, etc.)  */
				*header_subtitle = '~';

			}

		} else {

			/*  We don't have a path, but maybe we have a URI to show...  */

			header_subtitle = (
				tmpbuf = nautilus_file_info_get_uri(
					NAUTILUS_FILE_INFO(session->targets->data)
				)
			) ? tmpbuf : _("Unknown path");

		}

	}

	gtk_window_set_title(GTK_WINDOW(a8n_dialog), header_title);

	gtk_header_bar_set_subtitle(
		GTK_HEADER_BAR(gtk_dialog_get_header_bar(a8n_dialog)),
		header_subtitle
	);

	g_free(tmpbuf);

	GdkRectangle workarea = { 0 };

	gdk_monitor_get_workarea(
		gdk_display_get_monitor_at_window(
			gdk_display_get_default(),
			gtk_widget_get_window(GTK_WIDGET(parent))
		),
		&workarea
	);

	gtk_window_set_default_size(
		GTK_WINDOW(a8n_dialog),
		workarea.width ? workarea.width * 2 / 3 : A8N_WIN_FALLBACK_WIDTH,
		workarea.height ? workarea.height * 2 / 3 : A8N_WIN_FALLBACK_HEIGHT
	);

	GtkWidget
		* const scrollable = gtk_scrolled_window_new(NULL, NULL),
		* const text_area = gtk_source_view_new_with_buffer(
			session->annotation_text
		);

	gtk_style_context_add_class(
		gtk_widget_get_style_context(text_area),
		"nautilus-annotations-view"
	);

	gtk_widget_set_vexpand(text_area, true);
	gtk_widget_set_hexpand(text_area, true);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_area), GTK_WRAP_WORD);
	gtk_container_add(GTK_CONTAINER(scrollable), text_area);

	gtk_container_add(
		GTK_CONTAINER(gtk_dialog_get_content_area(a8n_dialog)),
		scrollable
	);

	g_signal_connect(
		G_OBJECT(a8n_dialog),
		"response",
		G_CALLBACK(on_annotation_dialog_response),
		session
	);

	g_signal_connect(
		G_OBJECT(session->annotation_text),
		"modified-changed",
		G_CALLBACK(on_text_modified_state_change),
		session
	);

	GtkAccelGroup * shortcuts = gtk_accel_group_new();

	gtk_accel_group_connect(
		shortcuts,
		GDK_KEY_S,
		GDK_CONTROL_MASK,
		0,
		g_cclosure_new_swap(
			G_CALLBACK(annotation_session_save),
			session,
			NULL
		)
	);

	gtk_accel_group_connect(
		shortcuts,
		GDK_KEY_Escape,
		GDK_SHIFT_MASK,
		0,
		g_cclosure_new_swap(
			G_CALLBACK(annotation_session_discard),
			session,
			NULL
		)
	);

	gtk_window_add_accel_group(GTK_WINDOW(a8n_dialog), shortcuts);
	gtk_widget_show_all(GTK_WIDGET(a8n_dialog));

}


static void on_annotate_menuitem_activate (
	NautilusMenuItem * const menu_item,
	GtkWindow * const nautilus_window
) {

	GList * const file_selection = g_object_get_data(
		G_OBJECT(menu_item),
		"nautilus_annotations_files"
	);

	if (!file_selection) {

		g_message("%s", _("No files were selected to be annotated"));
		return;

	}

	const gchar * a8n_probe;
	gchar * current_annotation = NULL;
	GFile * location;
	GFileInfo * finfo;
	GError * loaderr = NULL;
	gchar * uri;

	for (GList * iter = file_selection; iter; iter = iter->next) {

		location = nautilus_file_info_get_location(
			NAUTILUS_FILE_INFO(iter->data)
		);

		finfo = g_file_query_info(
			location,
			G_FILE_ATTRIBUTE_METADATA_ANNOTATION,
			G_FILE_QUERY_INFO_NONE,
			NULL,
			&loaderr
		);

		g_object_unref(location);

		if (!finfo) {

			uri = nautilus_file_info_get_uri(NAUTILUS_FILE_INFO(iter->data));

			g_message(
				"%s (%s) // %s",
				_("Could not access file's annotations"),
				uri ? uri : _("unknown path"),
				loaderr->message
			);

			g_free(uri);
			g_error_free(loaderr);
			goto free_and_exit;

		}

		a8n_probe = g_file_info_get_attribute_string(
			finfo,
			G_FILE_ATTRIBUTE_METADATA_ANNOTATION
		);

		if (!a8n_probe) {

			goto unref_and_continue;

		}

		if (!current_annotation) {

			current_annotation = g_strdup(a8n_probe);
			goto unref_and_continue;

		}

		if (strcmp(a8n_probe, current_annotation)) {

			g_object_unref(finfo);
			g_clear_pointer(&current_annotation, g_free);

			if (
				destructive_action_confirm(
					nautilus_window,
					_("At least two annotations in the file selection differ"),
					_("This will set up a blank new annotation."),
					NULL  /*  `NULL` defaults to `"_OK"`  */
				)
			) {

				break;

			}

			return;

		}


		/* \                                /\
		\ */     unref_and_continue:       /* \
		 \/     ______________________     \ */


		g_object_unref(finfo);

	}

	annotation_session_new_with_text(
		nautilus_window,
		file_selection,
		current_annotation
	);


	/* \                                /\
	\ */     free_and_exit:            /* \
	 \/     ______________________     \ */


	g_free(current_annotation);

}


static void on_unannotate_menuitem_activate (
	NautilusMenuItem * const menu_item,
	GtkWindow * const nautilus_window
) {

	GList * const file_selection = g_object_get_data(
		G_OBJECT(menu_item),
		"nautilus_annotations_files"
	);

	if (!file_selection) {

		g_message("%s", _("No files were selected to be unannotated"));
		return;

	}

	if (
		destructive_action_confirm(
			nautilus_window,
			_("Do you really want to erase the annotations attached?"),
			_("The annotations will be lost forever."),
			_("E_rase")
		)
	) {

		clear_files(file_selection);

	}

}


static GList * nautilus_annotations_get_file_items (
	NautilusMenuProvider * const menu_provider,
	GtkWidget * const nautilus_window,
	GList * const file_selection
) {

	UNUSED(menu_provider);

	#define NA_IS_FILE_SELECTION 1
	#define NA_IS_DIRECTORY_SELECTION 2
	#define NA_IS_MIXED_SELECTION 3

	#define NA_HAVE_ANNOTATED 1
	#define NA_HAVE_UNANNOTATED 2

	if (!file_selection) {

		return NULL;

	}

	guint8 selection_type = 0, selection_flags = 0;
	gsize sellen = 0;
	GList * iter;
	GFile * location;
	GFileInfo * finfo;

	for (iter = file_selection; iter; sellen++, iter = iter->next) {

		selection_type |= nautilus_file_info_is_directory(
			NAUTILUS_FILE_INFO(iter->data)
		) ? NA_IS_DIRECTORY_SELECTION : NA_IS_FILE_SELECTION;

		location = nautilus_file_info_get_location(
			NAUTILUS_FILE_INFO(iter->data)
		);

		finfo = g_file_query_info(
			location,
			G_FILE_ATTRIBUTE_METADATA_ANNOTATION,
			G_FILE_QUERY_INFO_NONE,
			NULL,
			NULL
		);

		g_object_unref(location);

		if (!finfo) {

			/*  File does not support annotations  */

			return NULL;

		}

		selection_flags |= g_file_info_get_attribute_string(
			finfo,
			G_FILE_ATTRIBUTE_METADATA_ANNOTATION
		) ? NA_HAVE_ANNOTATED : NA_HAVE_UNANNOTATED;

		g_object_unref(finfo);

		if (
			!(~selection_flags & (NA_HAVE_ANNOTATED | NA_HAVE_UNANNOTATED))
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

	NautilusMenuItem * item_annotations, * item_annotate;

	if (selection_flags & NA_HAVE_ANNOTATED) {

		NautilusMenu * const menu_annotations = nautilus_menu_new();
		NautilusMenuItem * subitem_iter;

		item_annotations = nautilus_menu_item_new(
			"NautilusAnnotations::choose",
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
			NULL
		);

		nautilus_menu_item_set_submenu(item_annotations, menu_annotations);

		item_annotate = nautilus_menu_item_new(
			"NautilusAnnotations::annotate",
			selection_flags & NA_HAVE_UNANNOTATED ?
				_("_Edit and extend")
			:
				_("_Edit"),
			selection_flags & NA_HAVE_UNANNOTATED ?
				_("Edit and extend the annotations attached to the selected"
					" objects")
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
			G_OBJECT(subitem_iter),
			"activate",
			G_CALLBACK(on_unannotate_menuitem_activate),
			nautilus_window
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
		G_OBJECT(item_annotate),
		"activate",
		G_CALLBACK(on_annotate_menuitem_activate),
		nautilus_window
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

	#undef NA_IS_MIXED_SELECTION
	#undef NA_IS_DIRECTORY_SELECTION
	#undef NA_IS_FILE_SELECTION

}


static GList * nautilus_annotations_get_background_items (
	NautilusMenuProvider * const menu_provider,
	GtkWidget * const nautilus_window,
	NautilusFileInfo * const current_folder
) {

	return nautilus_annotations_get_file_items(
		menu_provider,
		nautilus_window,
		&((GList) {
			.data = current_folder,
			.next = NULL,
			.prev = NULL
		})
	);

}


static NautilusOperationResult nautilus_annotations_update_file_info (
	NautilusInfoProvider * const info_provider,
	NautilusFileInfo * const nautilus_file,
	GClosure * const update_complete,
	NautilusOperationHandle ** const handle
) {

	UNUSED(info_provider);
	UNUSED(update_complete);
	UNUSED(handle);

	GFile * location = nautilus_file_info_get_location(nautilus_file);

	GFileInfo * const finfo = g_file_query_info(
		location,
		G_FILE_ATTRIBUTE_METADATA_ANNOTATION,
		G_FILE_QUERY_INFO_NONE,
		NULL,
		NULL
	);

	g_object_unref(location);

	if (!finfo) {

		return NAUTILUS_OPERATION_FAILED;

	}

	const gchar * a8n_probe = g_file_info_get_attribute_string(
		finfo,
		G_FILE_ATTRIBUTE_METADATA_ANNOTATION
	);

	if (a8n_probe) {

		nautilus_file_info_add_emblem(nautilus_file, "emblem-annotations");

		if (
			g_utf8_strlen(
				a8n_probe,
				strlen(a8n_probe)
			) > A8N_COLUMN_MAX_LENGTH
		) {

			gsize a8n_size = g_utf8_offset_to_pointer(
				a8n_probe,
				A8N_COLUMN_MAX_LENGTH
			) - a8n_probe;

			gchar * a8n_preview = g_malloc(a8n_size + 4);
			memcpy(a8n_preview, a8n_probe, a8n_size);
			memcpy(a8n_preview + a8n_size, "\342\200\246", 4);

			nautilus_file_info_add_string_attribute(
				nautilus_file,
				"annotations",
				a8n_preview
			);

			g_free(a8n_preview);

		} else {

			nautilus_file_info_add_string_attribute(
				nautilus_file,
				"annotations",
				a8n_probe
			);
		}

	} else {

		nautilus_file_info_add_string_attribute(
			nautilus_file,
			"annotations",
			""
		);

	}

	g_object_unref(finfo);
	return NAUTILUS_OPERATION_COMPLETE;

}


static GList * nautilus_annotations_get_columns (
	NautilusColumnProvider * const column_provider
) {

	UNUSED(column_provider);

	NautilusColumn * column = nautilus_column_new(
		"annotations",
		"annotations",
		_("Annotations"),
		_("Annotations attached to the object")
	);

	return g_list_append(NULL, column);

}


static void nautilus_annotations_type_info_provider_iface_init (
	NautilusInfoProviderIface * const iface,
	gpointer const iface_data
) {

	UNUSED(iface_data);

	iface->update_file_info = nautilus_annotations_update_file_info;

}


static void nautilus_annotations_menu_provider_iface_init (
	NautilusMenuProviderIface * const iface,
	gpointer const iface_data
) {

	UNUSED(iface_data);

	iface->get_file_items = nautilus_annotations_get_file_items;
	iface->get_background_items = nautilus_annotations_get_background_items;

}


static void nautilus_annotations_column_provider_iface_init (
	NautilusColumnProviderIface * const iface,
	gpointer const iface_data
) {

	UNUSED(iface_data);

	iface->get_columns = nautilus_annotations_get_columns;

}


static void nautilus_annotations_class_init (
	NautilusAnnotationsClass * const nautilus_annotations_class,
	gpointer const class_data
) {

	UNUSED(class_data);

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
		(GTypeValueTable *) NULL
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

	static const GInterfaceInfo column_provider_iface_info = {
		(GInterfaceInitFunc) nautilus_annotations_column_provider_iface_init,
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

	g_type_module_add_interface(
		module,
		nautilus_annotations_type,
		NAUTILUS_TYPE_COLUMN_PROVIDER,
		&column_provider_iface_info
	);

}


GType nautilus_annotations_get_type (void) {

	return nautilus_annotations_type;

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


void nautilus_module_initialize (
	GTypeModule * const module
) {

	I18N_INIT();
	nautilus_annotations_register_type(module);
	*provider_types = nautilus_annotations_get_type();
	annotations_css = gtk_css_provider_new();

	/*

	Search for the CSS in `~/.local/share/nautilus-annotations/` first; if not
	found assume that `/usr/share/nautilus-annotations/` is where the CSS will
	be found

	*/

	GFile * css_file = g_file_new_build_filename(
		g_get_user_data_dir(),
		PACKAGE_TARNAME,
		STYLESHEET_FILENAME,
		NULL
	);

	GFileInfo * finfo = g_file_query_info(
		css_file,
		G_FILE_ATTRIBUTE_ACCESS_CAN_READ,
		G_FILE_QUERY_INFO_NONE,
		NULL,
		NULL
	);

	enum {
		FILE_IS_GOOD,
		FILE_IS_MISSING,
		FILE_IS_UNREADABLE
	};

	switch(
		!finfo ? FILE_IS_MISSING
		: !g_file_info_get_attribute_boolean(
			finfo,
			G_FILE_ATTRIBUTE_ACCESS_CAN_READ
		) ? FILE_IS_UNREADABLE
		: FILE_IS_GOOD
	) {

		case FILE_IS_UNREADABLE:

			g_object_unref(finfo);
			/*  No case break (fallthrough)  */

		/* fallthrough */
		case FILE_IS_MISSING:

			g_object_unref(css_file);

			css_file = g_file_new_build_filename(
				PACKAGE_DATA_DIR,
				STYLESHEET_FILENAME,
				NULL
			);

			/*  No case break (fallthrough)  */

		/* fallthrough */
		default:

			gtk_css_provider_load_from_file(annotations_css, css_file, NULL);
			g_object_unref(css_file);

			gtk_style_context_add_provider_for_screen(
				gdk_display_get_default_screen(gdk_display_get_default()),
				GTK_STYLE_PROVIDER(annotations_css),
				GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
			);

			/*  No case break (last case)  */

	}

}


/*  EOF  */

