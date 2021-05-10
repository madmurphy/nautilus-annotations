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
#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>
#include <nautilus-extension.h>



/*\
|*|
|*| BUILD SETTINGS
|*|
\*/


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
|*| GLOBAL TYPES AND VARIABLES
|*|
\*/


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

#ifndef G_FILE_ATTRIBUTE_METADATA_ANNOTATION
/*  This metadata key was originally used by Nautilus  */
#define G_FILE_ATTRIBUTE_METADATA_ANNOTATION "metadata::annotation"
#endif

/*  The CSS to add to the screen  */
#define NAUTILUS_ANNOTATIONS_CSS PACKAGE_DATA_DIR "/style.css"



/*\
|*|
|*| FUNCTIONS
|*|
\*/


static bool destructive_action_confirm (
	GtkWindow * const parent,
	const char * const primary_text,
	const char * const secondary_text,
	/*  nullable  */  const char * const destructive_text
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


static void clear_files (
	GList * file_selection
) {

	GFile * location;
	GError * eraserr = NULL;

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

			fprintf(
				stderr,
				"Nautilus Annotations: %s // %s\n",
				_("Could not erase file's annotations"),
				eraserr->message
			);

			g_clear_error(&eraserr);

		}

		g_object_unref(location);

	}

}


static void annotation_session_destroy (
	NautilusAnnotationsSession * const session
) {

	gtk_widget_destroy(GTK_WIDGET(session->annotation_dialog));
	nautilus_file_info_list_free(session->targets);
	g_free(session);

}


guint annotation_session_export (
	NautilusAnnotationsSession * session
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
		clear_files(session->targets);
		return 0;

	}

	guint unsaved = 0;
	GFile * location;
	GError * saverr = NULL;

	for (
		GList * iter = session->targets;
			iter;
		iter = iter->next
	) {

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

			fprintf(
				stderr,
				"Nautilus Annotations: %s // %s\n",
				_("Could not save file's annotations"),
				saverr->message
			);

			g_clear_error(&saverr);
			unsaved++;

		}

		g_object_unref(location);

	}

	g_free(text_content);
	return unsaved;

}


void annotation_session_save (
	NautilusAnnotationsSession * session
) {

	if (!annotation_session_export(session)) {

		gtk_text_buffer_set_modified(
			GTK_TEXT_BUFFER(session->annotation_text),
			false
		);

	}

}


void annotation_session_exit (
	NautilusAnnotationsSession * session
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


void annotation_session_discard (
	NautilusAnnotationsSession * session
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

	switch (response_id) {

		case GTK_RESPONSE_REJECT: annotation_session_discard(session); return;
		default: annotation_session_exit(session);

	}

}


static void on_text_modified_state_change (
	GtkSourceBuffer * const text_buffer,
	NautilusAnnotationsSession * const session
) {

	switch (
		(gtk_text_buffer_get_modified(
			GTK_TEXT_BUFFER(text_buffer)
		) << 1) | !session->discard_button
	) {

		case 0:

			gtk_widget_destroy(GTK_WIDGET(session->discard_button));
			session->discard_button = NULL;
			break;

		case 3:

			session->discard_button = GTK_BUTTON(
				gtk_dialog_add_button(
					session->annotation_dialog,
					_("_Discard changes"),
					GTK_RESPONSE_REJECT
				)
			);

			gtk_style_context_add_class(
				gtk_widget_get_style_context(
					GTK_WIDGET(session->discard_button)
				),
				"nautilus-annotations-discard"
			);

		/*

		`case 1` ==> The `"modified"` bit flipped to `false` but for obscure
			reasons the discard button was already gone (do nothing)
		`case 2` ==> The `"modified"` bit flipped to `true` but for obscure
			reasons the discard button was already there (do nothing)

		*/

	}

}


static void annotation_session_new_with_text (
	GtkWindow * const parent,
	GList * const target_files,
	/*  nullable  */  gchar * initial_text
) {

	NautilusAnnotationsSession * const
		session = g_malloc(sizeof(NautilusAnnotationsSession));

	GtkDialog * anndialog = GTK_DIALOG(
		g_object_new(GTK_TYPE_DIALOG, "use-header-bar", true, NULL)
	);

	*session = (NautilusAnnotationsSession) {
		.annotation_dialog = anndialog,
		.annotation_text = gtk_source_buffer_new_with_language(
			gtk_source_language_manager_get_language(
				gtk_source_language_manager_get_default(),
				"markdown"
			)
		),
		.discard_button = NULL,
		.targets = nautilus_file_info_list_copy(target_files),
	};

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
		gtk_widget_get_style_context(GTK_WIDGET(anndialog)),
		"nautilus-annotations-dialog"
	);

	const gchar
		* header_title;

	gchar
		* header_subtitle,
		* tmpbuf = NULL;

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
		header_subtitle = _("Unknown path");

		if ((header_subtitle = tmpbuf)) {

			const gchar * const homedir = g_get_home_dir();

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

		}

	}

	gtk_window_set_title(GTK_WINDOW(anndialog), header_title);

	gtk_header_bar_set_subtitle(
		GTK_HEADER_BAR(gtk_window_get_titlebar(GTK_WINDOW(anndialog))),
		header_subtitle
	);

	g_free(tmpbuf);

	GtkWidget
		* const scrollable = gtk_scrolled_window_new(NULL, NULL),
		* const text_area = gtk_source_view_new_with_buffer(
			session->annotation_text
		);

	gtk_style_context_add_class(
		gtk_widget_get_style_context(text_area),
		"nautilus-annotations-view"
	);

	gtk_window_set_modal(GTK_WINDOW(anndialog), true);
	gtk_window_set_transient_for(GTK_WINDOW(anndialog), parent);

	GdkRectangle workarea = { 0 };

	gdk_monitor_get_workarea(
		gdk_display_get_monitor_at_window(
			gdk_display_get_default(),
			gtk_widget_get_window(GTK_WIDGET(parent))
		),
		&workarea
	);

	gtk_window_set_default_size(
		GTK_WINDOW(anndialog),
		workarea.width ? workarea.width * 2 / 3 : 300,
		workarea.height ? workarea.height * 2 / 3 : 400
	);

	gtk_widget_set_vexpand(text_area, true);
	gtk_widget_set_hexpand(text_area, true);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_area), GTK_WRAP_WORD);
	gtk_container_add(GTK_CONTAINER(scrollable), text_area);

	gtk_container_add(
		GTK_CONTAINER(gtk_dialog_get_content_area(anndialog)),
		scrollable
	);

	g_signal_connect(
		anndialog,
		"response",
		G_CALLBACK(on_annotation_dialog_response),
		session
	);

	g_signal_connect(
		session->annotation_text,
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

	gtk_window_add_accel_group(GTK_WINDOW(anndialog), shortcuts);
	gtk_widget_show_all(GTK_WIDGET(anndialog));

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

		fprintf(
			stderr,
			"Nautilus Annotations: %s\n",
			_("No files were selected to be annotated")
		);

		return;

	}

	const char * annotation_probe;
	gchar * current_annotation = NULL;
	GFile * location;
	GFileInfo * finfo;
	GError * loaderr = NULL;

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

			fprintf(
				stderr,
				"Nautilus Annotations: %s // %s\n",
				_("Could not access file's annotations"),
				loaderr->message
			);

			g_clear_error(&loaderr);
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

		fprintf(
			stderr,
			"Nautilus Annotations: %s\n",
			_("No files were selected to be unannotated")
		);

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

	#define NA_IS_FILE_SELECTION 1
	#define NA_IS_DIRECTORY_SELECTION 2

	#define NA_HAVE_ANNOTATED 1
	#define NA_HAVE_UNANNOTATED 2

	guint8
		selection_type = 0,
		selection_content = 0;

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
		item_annotate,
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

	#undef NA_IS_DIRECTORY_SELECTION
	#undef NA_IS_FILE_SELECTION

}


GList * nautilus_annotations_get_background_items (
	NautilusMenuProvider * const menu_provider,
	GtkWidget * const nautilus_window,
	NautilusFileInfo * const current_folder
) {

	GList
		* file_selection = g_list_append(NULL, current_folder),
		* menu_items = nautilus_annotations_get_file_items(
			menu_provider,
			nautilus_window,
			file_selection
		);

	g_list_free(file_selection);

	return menu_items;

}


NautilusOperationResult nautilus_annotations_update_file_info (
	NautilusInfoProvider * const info_provider,
	NautilusFileInfo * const file,
	GClosure * const update_complete,
	NautilusOperationHandle ** const handle
) {

	GFile * location = nautilus_file_info_get_location(file);

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


/*  EOF  */

