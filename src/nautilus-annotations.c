/*  -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*  Please make sure that the TAB width in your editor is set to 4 spaces  */

/*\
|*|
|*| nautilus-annotations.c
|*|
|*| https://gitlab.gnome.org/madmurphy/nautilus-annotations
|*|
|*| Copyright (C) 2021-2022 <madmurphy333@gmail.com>
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
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>
#include <nautilus-extension.h>
#include "emblem-picker/gnui-emblem-picker.h"



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


typedef struct NautilusAnnotations {
	GObject parent_slot;
} NautilusAnnotations;


typedef struct NautilusAnnotationsClass {
	GObjectClass parent_slot;
} NautilusAnnotationsClass;


typedef struct NautilusAnnotationsSession {
	GtkWindow * annotation_window;
	GtkSourceBuffer * annotation_text;
	GtkButton * discard_button;
	GtkRevealer * emblem_menu;
	GnuiEmblemPicker * emblem_picker;
	const GList * targets;
} NautilusAnnotationsSession;


typedef struct NautilusAnnotationsPreview {
	GtkWindow * parent;
	const GList * targets;
} NautilusAnnotationsPreview;


typedef struct NautilusAnnotationsDestroyFork {
	GDestroyNotify yes;
	GDestroyNotify no;
	gpointer data;
} NautilusAnnotationsDestroyFork;


static const char * const a8n_picker_forbidden_emblems[] = {
	"emblem-annotations",
	"emblem-annotations-symbolic", NULL
};


static const char * const a8n_resource_icon_paths[] = {
	"/org/gnome/nautilus/annotations/icons",
	NULL
};


static GType provider_types[1];
static GType nautilus_annotations_type;
static GObjectClass * parent_class;
static GtkApplication * nautilus_app;
static GtkCssProvider * a8n_fallback_css;
static GtkCssProvider * a8n_theme_css;
static GtkCssProvider * emblem_picker_css;


/*  This metadata key was originally used by Nautilus  */
#ifndef G_FILE_ATTRIBUTE_METADATA_ANNOTATION
#define G_FILE_ATTRIBUTE_METADATA_ANNOTATION "metadata::annotation"
#endif


/*  Default size of the annotation window when no workarea is available  */
#define A8N_WIN_FALLBACK_WIDTH 300
#define A8N_WIN_FALLBACK_HEIGHT 400


/*  These constants are measured in number of unicode characters  */
#define A8N_COLUMN_MAX_LENGTH 99
#define A8N_WINDOW_SUBTITLE_MAX_LENGTH 99


/*  Prefixes to tildize (to be provided by the build system in the future?)  */
#define A8N_USERS_HOME_PARENT "/home"
#define A8N_ROOT_USER_HOME "/root"



/*\
|*|
|*| FUNCTIONS
|*|
\*/


static void on_destructive_action_response (
	GtkDialog * const question_dialog,
	const int response,
	NautilusAnnotationsDestroyFork * const ifelse_fork
) {

	gtk_window_destroy(GTK_WINDOW(question_dialog));

	switch (
		response == GTK_RESPONSE_OK ?
			ifelse_fork->yes != NULL
		:
			(ifelse_fork->no != NULL) << 1
	) {

		case 1: ifelse_fork->yes(ifelse_fork->data); break;
		case 2: ifelse_fork->no(ifelse_fork->data);

	}

	g_free(ifelse_fork);

}


static void destructive_action_confirm (
	GtkWindow * const parent,
	const gchar * const primary_text,
	const gchar * const secondary_text,
	/*  nullable  */  const gchar * const destructive_text,
	const GDestroyNotify if_yes,
	const GDestroyNotify if_no,
	const gpointer ifelse_data
) {

	NautilusAnnotationsDestroyFork * const ifelse_fork =
		g_new(NautilusAnnotationsDestroyFork, 1);

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

	gtk_window_set_modal(GTK_WINDOW(question_dialog), true);
	ifelse_fork->yes = if_yes;
	ifelse_fork->no = if_no;
	ifelse_fork->data = ifelse_data;

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
	gtk_widget_add_css_class(destructive_button, "destructive-action");

	g_signal_connect(
		question_dialog,
		"response",
		G_CALLBACK(on_destructive_action_response),
		ifelse_fork
	);

	gtk_window_present(GTK_WINDOW(question_dialog));

}


static guint erase_annotations_in_files (
	const GList * const file_selection
) {

	GFile * location;
	GError * eraserr = NULL;
	guint noncleared = 0;
	gchar * uri;

	for (const GList * iter = file_selection; iter; iter = iter->next) {

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

			g_warning(
				"%s (%s) // %s",
				_("Could not erase file's annotations"),
				uri ? uri : _("unknown location"),
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


static void v_erase_annotations_in_files (
	const gpointer file_selection
) {

	erase_annotations_in_files(file_selection);

}


static void annotation_session_destroy (
	const gpointer session
) {

	gtk_window_destroy(
		GTK_WINDOW(((NautilusAnnotationsSession *) session)->annotation_window)
	);

	nautilus_file_info_list_free(
		(GList *) ((NautilusAnnotationsSession *) session)->targets
	);

	g_free(session);

}


static guint annotation_session_export (
	const NautilusAnnotationsSession * const session
) {

	GtkTextIter text_start, text_end;

	gtk_text_buffer_get_bounds(
		GTK_TEXT_BUFFER(session->annotation_text),
		&text_start,
		&text_end
	);

	gchar * const text_content = gtk_text_buffer_get_text(
		GTK_TEXT_BUFFER(session->annotation_text),
		&text_start,
		&text_end,
		false
	);

	if (!*text_content) {

		g_free(text_content);
		return erase_annotations_in_files(session->targets);

	}

	guint unsaved = 0;
	GFile * location;
	GError * saverr = NULL;
	gchar * uri;

	for (const GList * iter = session->targets; iter; iter = iter->next) {

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

			g_warning(
				"%s (%s) // %s",
				_("Could not save file's annotations"),
				uri ? uri : _("unknown location"),
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


static gboolean report_emblems_for_file (
	GFile * const location,
	const gchar * const * const added_emblems,
	const gchar * const * const removed_emblems G_GNUC_UNUSED,
	const GnuiEmblemPickerSaveResult result,
	const GError * const saverr,
	const gpointer user_data G_GNUC_UNUSED
) {

	NautilusFileInfo * nautilus_file;
	gchar * uri;
	const gchar * const * strptr;

	switch (result) {

		case GNUI_EMBLEM_PICKER_ERROR:

			uri = g_file_get_uri(location);

			g_warning(
				"%s (%s) // %s",
				_("Could not save file's emblems"),
				uri ? uri : _("unknown location"),
				saverr ? saverr->message : _("Unknown error")
			);

			g_free(uri);
			return true;

		case GNUI_EMBLEM_PICKER_SUCCESS:

			nautilus_file = nautilus_file_info_lookup(location);

			if (!nautilus_file) {

				uri = g_file_get_uri(location);

				g_warning(
					"%s (%s)",
					_("Could not refresh Nautilus emblems"),
					uri ? uri : _("unknown location")
				);

				g_free(uri);
				return true;

			}

			nautilus_file_info_invalidate_extension_info(nautilus_file);

			if ((strptr = added_emblems)) {

				while (*strptr) {

					nautilus_file_info_add_emblem(nautilus_file, *strptr++);

				}

			}

			g_object_unref(nautilus_file);

		/* fallthrough */
		default:

			return true;

	}

}


static void annotation_session_save (
	const NautilusAnnotationsSession * const session
) {

	if (!annotation_session_export(session)) {

		gtk_text_buffer_set_modified(
			GTK_TEXT_BUFFER(session->annotation_text),
			false
		);

	}

	gnui_emblem_picker_save(
		session->emblem_picker,
		report_emblems_for_file,
		GNUI_EMBLEM_PICKER_SAVE_FLAG_CALLBACK_ON_SUCCESS |
			GNUI_EMBLEM_PICKER_SAVE_FLAG_CALLBACK_ON_ERROR,
		NULL,
		NULL,
		NULL
	);

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

	if (gnui_emblem_picker_get_modified(session->emblem_picker)) {

		gnui_emblem_picker_save(
			session->emblem_picker,
			report_emblems_for_file,
			GNUI_EMBLEM_PICKER_SAVE_FLAG_CALLBACK_ON_SUCCESS |
				GNUI_EMBLEM_PICKER_SAVE_FLAG_CALLBACK_ON_ERROR,
			NULL,
			NULL,
			NULL
		);

	}

	annotation_session_destroy(session);

}


static void annotation_session_query_discard (
	NautilusAnnotationsSession * const session
) {

	if (
		gtk_text_buffer_get_modified(
			GTK_TEXT_BUFFER(session->annotation_text)
		) ||
		gnui_emblem_picker_get_modified(session->emblem_picker)
	) {

		destructive_action_confirm(
			GTK_WINDOW(session->annotation_window),
			_("Are you sure you want to discard the current changes?"),
			_("This action cannot be undone."),
			_("_Discard changes"),
			annotation_session_destroy,
			NULL,
			session
		);

	} else {

		annotation_session_destroy(session);

	}

}


static gboolean on_window_close (
	GtkWindow * const window G_GNUC_UNUSED,
	const gpointer session
) {

	annotation_session_exit(session);
	return false;

}


static void on_discard_button_clicked (
	NautilusAnnotationsSession * const session,
	GtkButton * const discard_button G_GNUC_UNUSED
) {

	annotation_session_query_discard(session);

}


static void on_modified_state_change (
	const NautilusAnnotationsSession * const session,
	GtkWidget * const changed_widget G_GNUC_UNUSED
) {

	gtk_widget_set_visible(
		GTK_WIDGET(session->discard_button),
		gtk_text_buffer_get_modified(
			GTK_TEXT_BUFFER(session->annotation_text)
		) || gnui_emblem_picker_get_modified(session->emblem_picker)
	);

}


static void on_exit_action_activate (
	GSimpleAction * const action G_GNUC_UNUSED,
	GVariant * const parameter G_GNUC_UNUSED,
	const gpointer v_session
) {

	#define session ((NautilusAnnotationsSession *) v_session)

	if (gtk_revealer_get_reveal_child(session->emblem_menu)) {

		gtk_revealer_set_reveal_child(session->emblem_menu, false);

	} else {

		annotation_session_exit(session);

	}

	#undef session

}


static void on_save_action_activate (
	GSimpleAction * const action G_GNUC_UNUSED,
	GVariant * const parameter G_GNUC_UNUSED,
	const gpointer session
) {

	annotation_session_save((NautilusAnnotationsSession *) session);

}


static void on_discard_action_activate (
	GSimpleAction * const action G_GNUC_UNUSED,
	GVariant * const parameter G_GNUC_UNUSED,
	const gpointer session
) {

	annotation_session_query_discard((NautilusAnnotationsSession *) session);

}


static void annotation_session_new_with_text (
	GtkWindow * const parent,
	const GList * const target_files,
	/*  nullable  */  const gchar * const initial_text
) {

	static const GActionEntry a8n_actions[] = {
		{
			.name = "exit_session",
			.activate = on_exit_action_activate
		}, {
			.name = "save_session",
			.activate = on_save_action_activate
		}, {
			.name = "discard_session",
			.activate = on_discard_action_activate
		},
	};

	NautilusAnnotationsSession
		* const session = g_new(NautilusAnnotationsSession, 1);

	GtkWindow * const a8n_window = GTK_WINDOW(
		g_object_new(
			GTK_TYPE_WINDOW,
			"modal", true,
			"transient-for", parent,
			NULL
		)
	);

	GdkRectangle workarea = { 0 };
	const gchar * header_title;
	gchar * header_subtitle, * tmpbuf = NULL;
	GSimpleActionGroup * const a8n_action_map = g_simple_action_group_new();
	GtkEventController * const a8n_controller = gtk_shortcut_controller_new();
	GtkWidget
		* _widget_placeholder_1_,
		* _widget_placeholder_2_,
		* _widget_placeholder_3_;

	session->annotation_window = a8n_window;
	session->targets = target_files;

	session->annotation_text = gtk_source_buffer_new_with_language(
		gtk_source_language_manager_get_language(
			gtk_source_language_manager_get_default(),
			"markdown"
		)
	);

	if (initial_text) {

		gtk_text_buffer_begin_irreversible_action(
			GTK_TEXT_BUFFER(session->annotation_text)
		);

		gtk_text_buffer_set_text(
			GTK_TEXT_BUFFER(session->annotation_text),
			initial_text,
			strlen(initial_text)
		);

		gtk_text_buffer_set_modified(
			GTK_TEXT_BUFFER(session->annotation_text),
			false
		);

		gtk_text_buffer_end_irreversible_action(
			GTK_TEXT_BUFFER(session->annotation_text)
		);

	}

	GList * emblem_picker_files = NULL;

	for (const GList * llnk = target_files; llnk; llnk = llnk->next) {

		emblem_picker_files = g_list_append(
			emblem_picker_files,
			nautilus_file_info_get_location(NAUTILUS_FILE_INFO(llnk->data))
		);

	}

	session->emblem_picker = GNUI_EMBLEM_PICKER(
		g_object_new(
			GNUI_TYPE_EMBLEM_PICKER,
			"mapped-files", emblem_picker_files,
			"reveal-changes", true,
			"forbidden-emblems", a8n_picker_forbidden_emblems,
			NULL
		)
	);

	g_list_free_full(emblem_picker_files, g_object_unref);

	gtk_widget_add_css_class(
		GTK_WIDGET(a8n_window),
		"nautilus-annotations-window"
	);

	if (session->targets->next) {

		header_title = _("Shared annotations");
		header_subtitle = _("(multiple objects)");

	} else {

		GFile * const location = nautilus_file_info_get_location(
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

			} else if (g_str_has_prefix(tmpbuf, A8N_USERS_HOME_PARENT)) {

				/*  Others (`~john/doc.md`, `~lisa/Videos`, etc.)  */
				*(header_subtitle += sizeof(A8N_USERS_HOME_PARENT) - 1) = '~';

			} else if (g_str_has_prefix(tmpbuf, A8N_ROOT_USER_HOME)) {

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

	if (
		g_utf8_strlen(
			header_subtitle,
			strlen(header_subtitle)
		) > A8N_WINDOW_SUBTITLE_MAX_LENGTH
	) {

		const gsize ellip_utf8_length = g_utf8_offset_to_pointer(
			header_subtitle,
			A8N_WINDOW_SUBTITLE_MAX_LENGTH
		) - header_subtitle;

		gchar * const ellipsis = g_malloc(ellip_utf8_length + 4);
		memcpy(ellipsis, header_subtitle, ellip_utf8_length);
		memcpy(ellipsis + ellip_utf8_length, "\342\200\246", 4);
		g_free(tmpbuf);
		header_subtitle = tmpbuf = ellipsis;

	}

	#define a8n_title_box _widget_placeholder_1_
	#define a8n_title_lbl _widget_placeholder_2_

	a8n_title_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	a8n_title_lbl = gtk_label_new(header_title);
	gtk_widget_add_css_class(a8n_title_lbl, "title");
	gtk_box_append(GTK_BOX(a8n_title_box), a8n_title_lbl);

	#undef a8n_title_lbl
	#define a8n_subtitle_lbl _widget_placeholder_2_

	a8n_subtitle_lbl = gtk_label_new(header_subtitle);
	gtk_widget_add_css_class(a8n_subtitle_lbl, "subtitle");
	gtk_box_append(GTK_BOX(a8n_title_box), a8n_subtitle_lbl);

	#undef a8n_subtitle_lbl
	#define a8n_header_bar _widget_placeholder_2_

	a8n_header_bar = gtk_header_bar_new();
	gtk_widget_set_valign(a8n_title_box, GTK_ALIGN_CENTER);

	gtk_header_bar_set_title_widget(
		GTK_HEADER_BAR(a8n_header_bar),
		a8n_title_box
	);

	g_free(tmpbuf);
	gtk_window_set_titlebar(a8n_window, a8n_header_bar);

	#undef a8n_title_box
	#define a8n_discard_btn _widget_placeholder_1_

	a8n_discard_btn = gtk_button_new_with_mnemonic(_("_Discard changes"));
	gtk_widget_set_valign(a8n_discard_btn, GTK_ALIGN_CENTER);

	g_signal_connect_swapped(
		a8n_discard_btn,
		"clicked",
		G_CALLBACK(on_discard_button_clicked),
		session
	);

	gtk_widget_hide(a8n_discard_btn);
	gtk_widget_add_css_class(a8n_discard_btn, "nautilus-annotations-discard");
	gtk_header_bar_pack_end(GTK_HEADER_BAR(a8n_header_bar), a8n_discard_btn);
	session->discard_button = GTK_BUTTON(a8n_discard_btn);

	#undef a8n_discard_btn
	#define a8n_emblems_btn _widget_placeholder_1_

	a8n_emblems_btn = gtk_toggle_button_new();

	gtk_button_set_icon_name(
		GTK_BUTTON(a8n_emblems_btn),
		"nautilus-annotations-tag-new"
	);

	gtk_widget_add_css_class(
		a8n_emblems_btn,
		"nautilus-annotations-add-emblem-button"
	);

	gtk_header_bar_pack_start(GTK_HEADER_BAR(a8n_header_bar), a8n_emblems_btn);

	#undef a8n_header_bar
	#define a8n_emblem_menu _widget_placeholder_2_

	a8n_emblem_menu = g_object_new(
		GTK_TYPE_REVEALER,
		"child", session->emblem_picker,
		"halign", GTK_ALIGN_START,
		"valign", GTK_ALIGN_START,
		NULL
	);

	g_object_bind_property(
		a8n_emblems_btn,
		"active",
		a8n_emblem_menu,
		"reveal-child",
		G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE
	);

	session->emblem_menu = GTK_REVEALER(a8n_emblem_menu);

	#undef a8n_emblems_btn
	#define a8n_overlay _widget_placeholder_1_
	#define scrollable _widget_placeholder_3_

	a8n_overlay = gtk_overlay_new();
	scrollable = gtk_scrolled_window_new();
	gtk_overlay_set_child(GTK_OVERLAY(a8n_overlay), scrollable);
	gtk_widget_add_css_class(a8n_emblem_menu, "nautilus-annotations-emblems");
	gtk_overlay_add_overlay(GTK_OVERLAY(a8n_overlay), a8n_emblem_menu);

	#undef a8n_emblem_menu
	#define text_area _widget_placeholder_2_

	gdk_monitor_get_geometry(
		gdk_display_get_monitor_at_surface(
			gdk_display_get_default(),
			gtk_native_get_surface(gtk_widget_get_native(GTK_WIDGET(parent)))
		),
		&workarea
	);

	gtk_window_set_default_size(
		a8n_window,
		workarea.width ? workarea.width * 2 / 3 : A8N_WIN_FALLBACK_WIDTH,
		workarea.height ? workarea.height * 2 / 3 : A8N_WIN_FALLBACK_HEIGHT
	);

	text_area = gtk_source_view_new_with_buffer(session->annotation_text);
	gtk_widget_add_css_class(text_area, "nautilus-annotations-view");
	gtk_widget_add_css_class(scrollable, "nautilus-annotations-scrollable");
	gtk_widget_set_vexpand(text_area, true);
	gtk_widget_set_hexpand(text_area, true);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_area), GTK_WRAP_WORD);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrollable), text_area);
	gtk_window_set_child(a8n_window, a8n_overlay);

	#undef scrollable
	#undef text_area
	#undef a8n_overlay

	g_signal_connect_swapped(
		session->annotation_text,
		"modified-changed",
		G_CALLBACK(on_modified_state_change),
		session
	);

	g_signal_connect_swapped(
		session->emblem_picker,
		"modified-changed",
		G_CALLBACK(on_modified_state_change),
		session
	);

	g_signal_connect(
		a8n_window,
		"close-request",
		G_CALLBACK(on_window_close),
		session
	);

	g_action_map_add_action_entries(
		G_ACTION_MAP(a8n_action_map),
		a8n_actions,
		G_N_ELEMENTS(a8n_actions),
		session
	);

	gtk_widget_insert_action_group(
		GTK_WIDGET(a8n_window),
		"a8n",
		G_ACTION_GROUP(a8n_action_map)
	);

	gtk_shortcut_controller_add_shortcut(
		GTK_SHORTCUT_CONTROLLER(a8n_controller),
		gtk_shortcut_new_with_arguments(
			gtk_keyval_trigger_new(GDK_KEY_S, GDK_CONTROL_MASK),
			gtk_named_action_new("a8n.save_session"),
			NULL
		)
	);

	gtk_shortcut_controller_add_shortcut(
		GTK_SHORTCUT_CONTROLLER(a8n_controller),
		gtk_shortcut_new_with_arguments(
			gtk_keyval_trigger_new(GDK_KEY_Escape, GDK_SHIFT_MASK),
			gtk_named_action_new("a8n.discard_session"),
			NULL
		)
	);

	gtk_shortcut_controller_add_shortcut(
		GTK_SHORTCUT_CONTROLLER(a8n_controller),
		gtk_shortcut_new_with_arguments(
			gtk_keyval_trigger_new(GDK_KEY_Escape, 0),
			gtk_named_action_new("a8n.exit_session"),
			NULL
		)
	);

	g_object_unref(a8n_action_map);
	gtk_widget_add_controller(GTK_WIDGET(a8n_window), a8n_controller);
	gtk_window_present(a8n_window);

}


static void annotation_session_preview_destroy (
	const gpointer session_preview
) {

	nautilus_file_info_list_free(
		(GList *) ((NautilusAnnotationsPreview *) session_preview)->targets
	);

	g_free(session_preview);

}


static void annotation_session_preview_ignite_and_destroy (
	const gpointer session_preview
) {

	annotation_session_new_with_text(
		((NautilusAnnotationsPreview *) session_preview)->parent,
		((NautilusAnnotationsPreview *) session_preview)->targets,
		NULL
	);

	g_free(session_preview);

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

	for (const GList * iter = file_selection; iter; iter = iter->next) {

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

			g_warning(
				"%s (%s) // %s",
				_("Could not access file's annotations"),
				uri ? uri : _("unknown location"),
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

			NautilusAnnotationsPreview * preview =
				g_new(NautilusAnnotationsPreview, 1);

			preview->parent = nautilus_window;
			preview->targets = nautilus_file_info_list_copy(file_selection);

			destructive_action_confirm(
				nautilus_window,
				_("At least two annotations in the file selection differ"),
				_("This will set up a blank new annotation."),
				NULL  /* `NULL` defaults to `"_OK"`  */,
				annotation_session_preview_ignite_and_destroy,
				annotation_session_preview_destroy,
				preview
			);

			g_object_unref(finfo);
			goto free_and_exit;

		}


		/* \                                /\
		\ */     unref_and_continue:       /* \
		 \/     ______________________     \ */


		g_object_unref(finfo);

	}

	annotation_session_new_with_text(
		nautilus_window,
		nautilus_file_info_list_copy(file_selection),
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

	const GList * const file_selection = g_object_get_data(
		G_OBJECT(menu_item),
		"nautilus_annotations_files"
	);

	if (!file_selection) {

		g_message("%s", _("No files were selected to be unannotated"));
		return;

	}

	destructive_action_confirm(
		nautilus_window,
		_("Do you really want to erase the annotations attached?"),
		_("The annotations will be lost forever."),
		_("E_rase"),
		v_erase_annotations_in_files,
		NULL,
		(gpointer) file_selection
	);

}


static GList * nautilus_annotations_get_file_items (
	NautilusMenuProvider * const menu_provider G_GNUC_UNUSED,
	GList * const file_selection
) {

	GtkWindow * nautilus_window =
		gtk_application_get_active_window(nautilus_app);

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
	const GList * iter;
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
			/*  Name  */
			"NautilusAnnotations::choose",
			/*  Label  */
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
			/*  Tip  */
			g_dngettext(
				GETTEXT_PACKAGE,
				"Choose an action for the object's annotations",
				"Choose an action for the objects' annotations",
				sellen
			),
			/*  Icon  */
			NULL
		);

		nautilus_menu_item_set_submenu(item_annotations, menu_annotations);

		item_annotate = nautilus_menu_item_new(
			/*  Name  */
			"NautilusAnnotations::annotate",
			/*  Label  */
			selection_flags & NA_HAVE_UNANNOTATED ?
				_("_Edit and extend")
			:
				_("_Edit"),
			/*  Tip  */
			selection_flags & NA_HAVE_UNANNOTATED ?
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
			/*  Icon  */
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
			/*  Name  */
			"NautilusAnnotations::annotate",
			/*  Label  */
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
			/*  Tip  */
			g_dngettext(
				GETTEXT_PACKAGE,
				"Attach an annotation to the selected object",
				"Attach an annotation to the selected objects",
				sellen
			),
			/*  Icon  */
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

	#undef NA_IS_MIXED_SELECTION
	#undef NA_IS_DIRECTORY_SELECTION
	#undef NA_IS_FILE_SELECTION

}


static GList * nautilus_annotations_get_background_items (
	NautilusMenuProvider * const menu_provider,
	NautilusFileInfo * const current_folder
) {

	return nautilus_annotations_get_file_items(
		menu_provider,
		&((GList) {
			.data = current_folder,
			.next = NULL,
			.prev = NULL
		})
	);

}


static NautilusOperationResult nautilus_annotations_update_file_info (
	NautilusInfoProvider * const info_provider G_GNUC_UNUSED,
	NautilusFileInfo * const nautilus_file,
	GClosure * const update_complete G_GNUC_UNUSED,
	NautilusOperationHandle ** const handle G_GNUC_UNUSED
) {

	GFile * const location = nautilus_file_info_get_location(nautilus_file);

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

	const gchar * const a8n_probe = g_file_info_get_attribute_string(
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

			const gsize a8n_utf8_size = g_utf8_offset_to_pointer(
				a8n_probe,
				A8N_COLUMN_MAX_LENGTH
			) - a8n_probe;

			gchar * a8n_preview = g_malloc(a8n_utf8_size + 4);
			memcpy(a8n_preview, a8n_probe, a8n_utf8_size);
			memcpy(a8n_preview + a8n_utf8_size, "\342\200\246", 4);

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
	NautilusColumnProvider * const column_provider G_GNUC_UNUSED
) {

	return g_list_append(
		NULL, 
		nautilus_column_new(
			"annotations",
			"annotations",
			_("Annotations"),
			_("Annotations attached to the object")
		)
	);

}


static void nautilus_annotations_type_info_provider_iface_init (
	NautilusInfoProviderInterface * const iface,
	const gpointer iface_data G_GNUC_UNUSED
) {

	iface->update_file_info = nautilus_annotations_update_file_info;

}


static void nautilus_annotations_menu_provider_iface_init (
	NautilusMenuProviderInterface * const iface,
	const gpointer iface_data G_GNUC_UNUSED
) {

	iface->get_file_items = nautilus_annotations_get_file_items;
	iface->get_background_items = nautilus_annotations_get_background_items;

}


static void nautilus_annotations_column_provider_iface_init (
	NautilusColumnProviderInterface * const iface,
	const gpointer iface_data G_GNUC_UNUSED
) {

	iface->get_columns = nautilus_annotations_get_columns;

}


static void nautilus_annotations_class_init (
	NautilusAnnotationsClass * const nautilus_annotations_class,
	const gpointer class_data G_GNUC_UNUSED
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

	GdkDisplay * const display = gdk_display_get_default();

	gtk_style_context_remove_provider_for_display(
		display,
		GTK_STYLE_PROVIDER(emblem_picker_css)
	);

	gtk_style_context_remove_provider_for_display(
		display,
		GTK_STYLE_PROVIDER(a8n_fallback_css)
	);

	g_object_unref(emblem_picker_css);
	g_object_unref(a8n_fallback_css);

	if (a8n_theme_css) {

		gtk_style_context_remove_provider_for_display(
			display,
			GTK_STYLE_PROVIDER(a8n_theme_css)
		);

		g_object_unref(a8n_theme_css);

	}

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

	GdkDisplay * const display = gdk_display_get_default();

	I18N_INIT();
	nautilus_annotations_register_type(module);
	*provider_types = nautilus_annotations_get_type();
	nautilus_app = GTK_APPLICATION(g_application_get_default());

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

	enum {
		A8N_CSS_FILE_FLAG_NONE = 0,
		A8N_CSS_FILE_FLAG_SYSTEM_THEME = true,
		A8N_CSS_FILE_FLAG_MISSING = 2,
		A8N_CSS_FILE_FLAG_UNREADABLE = 4
	};

	GFileInfo * finfo;
	bool b_try_system_theme = false;


	/* \                                /\
	\ */     load_theme_css:           /* \
	 \/     ______________________     \ */


	finfo = g_file_query_info(
		css_file,
		G_FILE_ATTRIBUTE_ACCESS_CAN_READ,
		G_FILE_QUERY_INFO_NONE,
		NULL,
		NULL
	);

	switch(
		!finfo ? A8N_CSS_FILE_FLAG_MISSING | b_try_system_theme
		: !g_file_info_get_attribute_boolean(
			finfo,
			G_FILE_ATTRIBUTE_ACCESS_CAN_READ
		) ? A8N_CSS_FILE_FLAG_UNREADABLE | b_try_system_theme
		: A8N_CSS_FILE_FLAG_NONE | b_try_system_theme
	) {

		case A8N_CSS_FILE_FLAG_UNREADABLE:

			g_object_unref(finfo);

		/* fallthrough */
		case A8N_CSS_FILE_FLAG_MISSING:

			g_object_unref(css_file);

			css_file = g_file_new_build_filename(
				PACKAGE_DATA_DIR,
				STYLESHEET_FILENAME,
				NULL
			);

			b_try_system_theme = true;
			goto load_theme_css;

		case A8N_CSS_FILE_FLAG_UNREADABLE | A8N_CSS_FILE_FLAG_SYSTEM_THEME:

			g_object_unref(finfo);

		/* fallthrough */
		case A8N_CSS_FILE_FLAG_MISSING | A8N_CSS_FILE_FLAG_SYSTEM_THEME:

			g_object_unref(css_file);
			break;

		default:

			a8n_theme_css = gtk_css_provider_new();
			gtk_css_provider_load_from_file(a8n_theme_css, css_file);
			g_object_unref(css_file);

			gtk_style_context_add_provider_for_display(
				display,
				GTK_STYLE_PROVIDER(a8n_theme_css),
				GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
			);

			/*  No case break (last case)  */

	}

	gtk_icon_theme_set_resource_path(
		gtk_icon_theme_get_for_display(display),
		a8n_resource_icon_paths
	);

	a8n_fallback_css = gtk_css_provider_new();

	gtk_css_provider_load_from_resource(
		a8n_fallback_css,
		"/org/gnome/nautilus/annotations/style.css"
	);

	gtk_style_context_add_provider_for_display (
		display,
		GTK_STYLE_PROVIDER(a8n_fallback_css),
		GTK_STYLE_PROVIDER_PRIORITY_THEME
	);

	emblem_picker_css = gtk_css_provider_new();

	gtk_css_provider_load_from_resource(
		emblem_picker_css,
		"/org/gnome/nautilus/annotations/emblem-picker/style.css"
	);

	gtk_style_context_add_provider_for_display (
		display,
		GTK_STYLE_PROVIDER(emblem_picker_css),
		GTK_STYLE_PROVIDER_PRIORITY_THEME
	);

}


/*  EOF  */

