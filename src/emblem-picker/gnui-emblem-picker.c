/*  -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*  Please make sure that the TAB width in your editor is set to 4 spaces  */

/*\
|*|
|*| gnui-emblem-picker.c
|*|
|*| https://github.com/madmurphy/libgnuisance
|*|
|*| Copyright (C) 2022 <madmurphy333@gmail.com>
|*|
|*| **libgnuisance** is free software: you can redistribute it and/or modify it
|*| under the terms of the GNU General Public License as published by the Free
|*| Software Foundation, either version 3 of the License, or (at your option)
|*| any later version.
|*|
|*| **libgnuisance** is distributed in the hope that it will be useful, but
|*| WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
|*| or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
|*| more details.
|*|
|*| You should have received a copy of the GNU General Public License along
|*| with this program. If not, see <http://www.gnu.org/licenses/>.
|*|
\*/



#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdbool.h>
#include <gtk/gtk.h>
#include <adwaita.h>
#include "gnui-definitions.h"
#include "gnui-internals.h"
#include "gnui-emblem-picker.h"



/*\
|*|
|*| LOCAL DEFINITIONS
|*|
\*/


#ifndef G_FILE_ATTRIBUTE_METADATA_EMBLEMS
/**

    G_FILE_ATTRIBUTE_METADATA_EMBLEMS:

    The attribute string for emblems

**/
#define G_FILE_ATTRIBUTE_METADATA_EMBLEMS "metadata::emblems"
#endif


/**

    GNUI_EMBLEM_PICKER_ICON_SIZE:

    The emblem picker's icon size

**/
#define GNUI_EMBLEM_PICKER_ICON_SIZE 32


/**

    GNUI_EMBLEM_PICKER_PAGE_MIN_WIDTH:

    The emblem view's minimum width

**/
#define GNUI_EMBLEM_PICKER_PAGE_MIN_WIDTH 256


/**

    GNUI_EMBLEM_PICKER_PAGE_MIN_HEIGHT:

    The emblem view's minimum height

**/
#define GNUI_EMBLEM_PICKER_PAGE_MIN_HEIGHT 256


/**

    GNUI_INDICATOR_GLYPH_DOWN_TO_SOME:

    The indicator glyph for inconsistent state after selected

    Currently set to `U+2702`, "BLACK SCISSORS"

**/
#define GNUI_INDICATOR_GLYPH_DOWN_TO_SOME "\342\234\202"


/**

    GNUI_INDICATOR_GLYPH_CHECK_YES:

    The indicator glyph for selected after non-selected

    Currently set to `U+2714`, "HEAVY CHECK MARK"

**/
#define GNUI_INDICATOR_GLYPH_CHECK_YES "\342\234\224"


/**

    GNUI_INDICATOR_GLYPH_CHECK_NO:

    The indicator glyph for non-selected after selected

    Currently set to `U+2718`, "HEAVY BALLOT"

**/
#define GNUI_INDICATOR_GLYPH_CHECK_NO "\342\234\230"


/**

    GNUI_INDICATOR_GLYPH_UP_TO_SOME:

    The indicator glyph for inconsistent state after non-selected

    Currently set to `U+2762`, "HEAVY EXCLAMATION MARK ORNAMENT"

**/
#define GNUI_INDICATOR_GLYPH_UP_TO_SOME "\342\235\242"


/**

    GNUI_INDICATOR_GLYPH_BLANK:

    The blank indicator glyph

    Currently set to an empty string

**/
#define GNUI_INDICATOR_GLYPH_BLANK ""


/**

    GNUI_TYPE_EMBLEM_PICKER_EMBLEM:

    A `GType` for `GnuiEmblemPickerEmblem`

**/
#define GNUI_TYPE_EMBLEM_PICKER_EMBLEM (gnui_emblem_picker_emblem_get_type())


/**

    GnuiEmblemPickerEmblem:

    A widget derived from `GtkFlowBoxChild` used only internally

**/
G_DECLARE_FINAL_TYPE(
    GnuiEmblemPickerEmblem,
    gnui_emblem_picker_emblem,
    GNUI,
    EMBLEM_PICKER_EMBLEM,
    GtkFlowBoxChild
)



/*\
|*|
|*| GLOBAL TYPES AND VARIABLES
|*|
\*/


/**

    GNUI_EMBLEM_PICKER_STATE_TRANSLATIONS:

    An array of three `GtkStateFlags` of interest, in the right order

**/
static const GtkStateFlags GNUI_EMBLEM_PICKER_STATE_TRANSLATIONS[] =
	{ GNUI_EMBLEM_STATES_WITH_PREFIX(GTK_STATE_FLAG_) };


/**

    GNUI_EMBLEM_PICKER_GTK_STATES_ALL_NOT:

    The bitwise negation of the three `GtkStateFlags` of interest

**/
#define GNUI_EMBLEM_PICKER_GTK_STATES_ALL_NOT ~( \
		GTK_STATE_FLAG_NORMAL | \
		GTK_STATE_FLAG_SELECTED | \
		GTK_STATE_FLAG_INCONSISTENT \
	)


/**

    GnuiEmblemPickerPageNum:

    The two page identifiers of the emblem picker's leaflet

**/
typedef enum _GnuiEmblemPickerPageNum {
	/*  Picker pages  */
	PAGE_SUPPORTED,
	PAGE_UNSUPPORTED,
	/*  Maximum number of picker pages  */
	N_EMBLEM_PICKER_PAGES
} GnuiEmblemPickerPageNum;


/**

    EmblemReference:

    Information about a single emblem in the emblem picker

**/
typedef struct _EmblemReference {
	gchar * name;
	GtkWidget * controller_cell;
	GtkLabel * change_indicator;
	GList * inconsistent_group;
	GnuiEmblemState saved_state;
	GnuiEmblemState current_state;
	bool unsupported;
} EmblemReference;


/**

    GnuiEmblemPicker:

    The emblem picker `struct`

**/
struct _GnuiEmblemPicker {
	GtkWidget parent_instance;
	GList * mapped_files;
	gchar ** forbidden_emblems;
	bool
		ensure_standard : 1,
		modified : 1,
		reveal_changes : 1;
};


/**

    GnuiEmblemPickerPage:

    Recurrent elements of interest in each page of the leaflet

**/
typedef struct _GnuiEmblemPickerPage {
	AdwLeaflet * pager;
	GtkWidget
		* emblem_view,
		* search_box,
		* search_entry;
	AdwNavigationDirection way_in;
} GnuiEmblemPickerPage;


/**

    GnuiEmblemPickerPrivate:

    The emblem picker's private `struct`

**/
typedef struct _GnuiEmblemPickerPrivate {
	GnuiEmblemPickerPage pages[N_EMBLEM_PICKER_PAGES];
	GtkIconTheme * icon_theme;
	GtkWidget
		* current_view,
		* two_pages,
		* single_page,
		* page_1_of_2,
		* supported_container;
	GList * references;
	gulong icon_theme_refresh_signal;
	bool is_single_page;
} GnuiEmblemPickerPrivate;


/**

    GnuiEmblemPickerEmblem:

    The emblem picker emblem `struct`

**/
struct _GnuiEmblemPickerEmblem {
	GtkFlowBoxChild parent_instance;
	EmblemReference * emref;
};


G_DEFINE_FINAL_TYPE_WITH_PRIVATE(
	GnuiEmblemPicker,
	gnui_emblem_picker,
	GTK_TYPE_WIDGET
)


G_DEFINE_FINAL_TYPE(
	GnuiEmblemPickerEmblem,
	gnui_emblem_picker_emblem,
	GTK_TYPE_FLOW_BOX_CHILD
)


enum {

	/*  Reserved for GObject  */
	RESERVED_PROPERTY = 0,

	/*  Properties  */
	PROPERTY_MAPPED_FILES,
	PROPERTY_FORBIDDEN_EMBLEMS,
	PROPERTY_ENSURE_STANDARD,
	PROPERTY_MODIFIED,
	PROPERTY_REVEAL_CHANGES,

	/*  Number of properties  */
	N_PROPERTIES

};


enum {

	/*  Signals  */
	SIGNAL_MODIFIED_CHANGED,
	SIGNAL_EMBLEM_SELECTED,

	/*  Number of signals  */
	N_SIGNALS

};


/**

    xdg_emblems:

    The list of standard emblems

    See
    https://specifications.freedesktop.org/icon-naming-spec/icon-naming-spec-0.4.html#idm45098400102912

**/
static const gchar * const xdg_emblems[] = {
	"emblem-default",
	"emblem-documents",
	"emblem-downloads",
	"emblem-favorites",
	"emblem-important",
	"emblem-mail",
	"emblem-photos",
	"emblem-readonly",
	"emblem-shared",
	"emblem-symbolic-link",
	"emblem-synchronized",
	"emblem-system",
	"emblem-unreadable"
};


/**

    N_XDG_EMBLEMS:

    The amount of standard emblems

**/
#define N_XDG_EMBLEMS (sizeof(xdg_emblems) / sizeof(const gchar *))


static GParamSpec * props[N_PROPERTIES];


static guint signals[N_SIGNALS];



/*\
|*|
|*| PRIVATE FUNCTIONS
|*|
\*/


/*  Inline  */


/**

    gnui_set_emblem_cell_gtk_state:
    @emblem_cell:   (not nullable): The emblem cell (a `GnuiEmblemPickerEmblem`
                    passed as `GtkWidget`)
    @state:         The current state of the emblem

    Update the `GtkState` of an emblem cell

**/
static inline void gnui_set_emblem_cell_gtk_state (
	GtkWidget * const emblem_cell,
	const GnuiEmblemState state
) {
	gtk_widget_set_state_flags(
		emblem_cell,
		GNUI_EMBLEM_PICKER_STATE_TRANSLATIONS[state] | (
			gtk_widget_get_state_flags(emblem_cell) &
			GNUI_EMBLEM_PICKER_GTK_STATES_ALL_NOT
		),
		true
	);
}


/**

    get_available_emblems:
    @self:      (not nullable): The emblem picker
    @priv:      (not nullable): The emblem picker's private `struct`

    Get the emblems available in the current theme

**/
G_GNUC_WARN_UNUSED_RESULT static GList * get_available_emblems (
	GnuiEmblemPicker * const self,
	GnuiEmblemPickerPrivate * const priv
) {

	GList * emblems = NULL;
	const gchar ** theme_icons, ** arrptr;
	gchar * icon_name;

	arrptr = theme_icons =
		(const gchar **) gtk_icon_theme_get_icon_names(priv->icon_theme);

	while ((icon_name = (gchar *) *arrptr++)) {

		if (
			!g_str_has_prefix(icon_name, "emblem-") || (
				self->forbidden_emblems && 
				g_strv_contains(
					(const gchar * const *) self->forbidden_emblems,
					icon_name
				)
			)
		) {

			g_free(icon_name);

		} else {

			emblems = g_list_prepend(emblems, icon_name);

		}

	}

	g_free(theme_icons);
	return emblems;

}


/*  Callbacks  */


/**

    gnui_emblem_picker_sort_emblem_references:
    @ref_a:     (auto) (not nullable): The first emblem reference
    @ref_b:     (auto) (not nullable): The second emblem reference

    Sort two emblem references

    Returns:    Exactly like `strcmp()`

**/
static int gnui_emblem_picker_sort_emblem_references (
	const EmblemReference * const ref_a,
	const EmblemReference * const ref_b
) {

	return strcmp(ref_a->name, ref_b->name);

}


/**

    gnui_emblem_picker_sort_emblem_cells:
    @cell_a:    (auto) (not nullable): The first emblem cell
    @cell_b:    (auto) (not nullable): The second emblem cell
    @data:      (auto) (unused): The closure data

    Sort two emblem cells

    Returns:    Exactly like `strcmp()`

**/
static int gnui_emblem_picker_sort_emblem_cells (
	GtkFlowBoxChild * const cell_a,
	GtkFlowBoxChild * const cell_b,
	const gpointer data G_GNUC_UNUSED
) {

	return strcmp(
		GNUI_EMBLEM_PICKER_EMBLEM(cell_a)->emref->name,
		GNUI_EMBLEM_PICKER_EMBLEM(cell_b)->emref->name
	);

}


/**

    gnui_emblem_picker_filter_emblem_cell:
    @emblem_cell:   (auto) (not nullable): The emblem cell (a
                    `GnuiEmblemPickerEmblem` passed as `GtkFlowBoxChild`)
    @v_page:        (auto) (not nullable): The current `GnuiEmblemPickerPage`
                    passed as `gpointer`

    Decide whether an emblem cell must be shown or not

    Returns:    `true` if the cell must be shown, `false` otherwise 

**/
static gboolean gnui_emblem_picker_filter_emblem_cell (
	GtkFlowBoxChild * const emblem_cell,
	const gpointer v_page
) {

	#define page ((GnuiEmblemPickerPage *) v_page)

	return
		!gtk_revealer_get_reveal_child(
			GTK_REVEALER(page->search_box)
		) || g_strrstr(
			GNUI_EMBLEM_PICKER_EMBLEM(emblem_cell)->emref->name,
			gtk_editable_get_text(GTK_EDITABLE(page->search_entry))
		);

	#undef page

}


/*  Implementation  */


/**

    gnui_emblem_picker_refresh_cell:
    @emref:     (not nullable): The emblem reference

    Refresh an emblem picker cell

**/
static void gnui_emblem_picker_refresh_cell (
	const EmblemReference * const emref
) {

	const GnuiEmblemState state = emref->current_state;

	gnui_set_emblem_cell_gtk_state(emref->controller_cell, state);

	gtk_label_set_text(
		emref->change_indicator,
		emref->saved_state == GNUI_EMBLEM_STATE_SELECTED ?
			(
				state == GNUI_EMBLEM_STATE_SELECTED ?
					GNUI_INDICATOR_GLYPH_BLANK
				: state == GNUI_EMBLEM_STATE_INCONSISTENT ?
					GNUI_INDICATOR_GLYPH_DOWN_TO_SOME
				:
					GNUI_INDICATOR_GLYPH_CHECK_NO
			)
		: emref->saved_state == GNUI_EMBLEM_STATE_INCONSISTENT ?
			(
				state == GNUI_EMBLEM_STATE_SELECTED ?
					GNUI_INDICATOR_GLYPH_CHECK_YES
				: state == GNUI_EMBLEM_STATE_INCONSISTENT ?
					GNUI_INDICATOR_GLYPH_BLANK
				:
					GNUI_INDICATOR_GLYPH_CHECK_NO
			)
		: state == GNUI_EMBLEM_STATE_SELECTED ?
			GNUI_INDICATOR_GLYPH_CHECK_YES
		: state == GNUI_EMBLEM_STATE_INCONSISTENT ?
			GNUI_INDICATOR_GLYPH_UP_TO_SOME
		:
			GNUI_INDICATOR_GLYPH_BLANK
		);

}


/**

    gnui_emblem_picker_toggle_emblem_reference:
    @self:      (not nullable): The emblem picker
    @emref:     (not nullable): The emblem reference

    Toggle/untoggle an emblem picker cell

**/
static void gnui_emblem_picker_toggle_emblem_reference (
	GnuiEmblemPicker * const self,
	EmblemReference * const emref
) {

	const GnuiEmblemPickerPrivate * const priv =
		gnui_emblem_picker_get_instance_private(self);

	gboolean is_modified;

	emref->current_state =
		emref->inconsistent_group &&
		emref->current_state == GNUI_EMBLEM_STATE_SELECTED  ?
			GNUI_EMBLEM_STATE_INCONSISTENT
		: emref->current_state == GNUI_EMBLEM_STATE_NORMAL ?
			GNUI_EMBLEM_STATE_SELECTED
		:
			GNUI_EMBLEM_STATE_NORMAL;

	gnui_emblem_picker_refresh_cell(emref);

	if (emref->current_state == emref->saved_state) {

		const GList * refllnk = priv->references;

		find_emblem_reference: if (refllnk) {

			#define iter_emref ((EmblemReference *) refllnk->data)

			if (iter_emref->current_state == iter_emref->saved_state) {

				refllnk = refllnk->next;
				goto find_emblem_reference;

			}

			is_modified = true;

			#undef iter_emref

		} else {

			is_modified = false;

		}

	} else {

		is_modified = true;

	}

	if (self->modified != is_modified) {

		self->modified = is_modified;
		g_object_notify_by_pspec(G_OBJECT(self), props[PROPERTY_MODIFIED]);
		g_signal_emit(self, signals[SIGNAL_MODIFIED_CHANGED], 0, is_modified);

	}

	g_signal_emit(
		self,
		signals[SIGNAL_EMBLEM_SELECTED],
		0,
		emref->name,
		emref->saved_state,
		emref->current_state,
		emref->inconsistent_group
	);

}


/**

    gnui_emblem_picker_reference_destroy:
    @priv:      (not nullable): The emblem picker's private `struct`
    @emref:     (not nullable): The emblem reference to destroy

    Destroy an emblem picker cell

**/
static void gnui_emblem_picker_reference_destroy (
	const GnuiEmblemPickerPrivate * const priv,
	EmblemReference * const emref
) {

	gtk_flow_box_remove(
		GTK_FLOW_BOX(priv->pages[emref->unsupported].emblem_view),
		emref->controller_cell
	);

	g_free(emref->name);
	g_list_free(emref->inconsistent_group);
	g_free(emref);

}


/**

    gnui_emblem_picker_draw_supported_emblem:
    @self:      (not nullable): The emblem picker
    @priv:      (not nullable): The emblem picker's private `struct`
    @emref:     (not nullable): The emblem reference

    Visualize an emblem in the supported emblem list

**/
static void gnui_emblem_picker_draw_supported_emblem (
	GnuiEmblemPicker * const self,
	const GnuiEmblemPickerPrivate * const priv,
	EmblemReference * const emref
) {

	GtkWidget * _widget_placeholder_1_, * _widget_placeholder_2_;

	#define emblem_container _widget_placeholder_1_
	#define emblem_image _widget_placeholder_2_

	emblem_container = gtk_overlay_new();

	emblem_image = g_object_new(
		GTK_TYPE_IMAGE,
		"icon-name", emref->name,
		"pixel-size", GNUI_EMBLEM_PICKER_ICON_SIZE,
		"tooltip-text", emref->name,
		NULL
	);

	gtk_widget_add_css_class(emblem_image, "emblem-image");
	gtk_overlay_set_child(GTK_OVERLAY(emblem_container), emblem_image);

	#undef emblem_image
	#define indicator _widget_placeholder_2_

	indicator = g_object_new(
		GTK_TYPE_LABEL,
		"halign", GTK_ALIGN_START,
		"valign", GTK_ALIGN_START,
		NULL
	);

	g_object_bind_property(
		self,
		"reveal-changes",
		indicator,
		"visible",
		G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE
	);

	gtk_widget_add_css_class(indicator, "change-indicator");
	emref->change_indicator = GTK_LABEL(indicator);
	gtk_overlay_add_overlay(GTK_OVERLAY(emblem_container), indicator);

	#undef indicator
	#define emblem_cell _widget_placeholder_2_

	emblem_cell = g_object_new(
		GNUI_TYPE_EMBLEM_PICKER_EMBLEM,
		"child", emblem_container,
		NULL
	);

	gnui_set_emblem_cell_gtk_state(emblem_cell, emref->current_state);
	gtk_widget_add_css_class(emblem_cell, "supported");
	GNUI_EMBLEM_PICKER_EMBLEM(emblem_cell)->emref = emref;
	emref->controller_cell = emblem_cell;

	gtk_flow_box_insert(
		GTK_FLOW_BOX(priv->pages[PAGE_SUPPORTED].emblem_view),
		emblem_cell,
		-1
	);

	#undef emblem_cell
	#undef emblem_container

}


/**

    gnui_emblem_picker_draw_unsupported_emblem:
    @self:      (not nullable): The emblem picker
    @priv:      (not nullable): The emblem picker's private `struct`
    @emref:     (not nullable): The emblem reference

    Visualize an emblem in the unsupported emblem list

**/
static void gnui_emblem_picker_draw_unsupported_emblem (
	GnuiEmblemPicker * const self,
	const GnuiEmblemPickerPrivate * const priv,
	EmblemReference * const emref
) {

	GtkWidget * _widget_placeholder_1_, * _widget_placeholder_2_;

	#define emblem_container _widget_placeholder_1_
	#define emblem_label _widget_placeholder_2_

	emblem_container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

	emblem_label = g_object_new(
		GTK_TYPE_LABEL,
		"label", emref->name,
		"halign", GTK_ALIGN_START,
		"hexpand", true,
		NULL
	);

	gtk_widget_add_css_class(emblem_label, "emblem-name");
	gtk_box_prepend(GTK_BOX(emblem_container), emblem_label);

	#undef emblem_label
	#define indicator _widget_placeholder_2_

	indicator = g_object_new(
		GTK_TYPE_LABEL,
		"halign", GTK_ALIGN_CENTER,
		"valign", GTK_ALIGN_CENTER,
		NULL
	);

	g_object_bind_property(
		self,
		"reveal-changes",
		indicator,
		"visible",
		G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE
	);

	gtk_widget_add_css_class(indicator, "change-indicator");
	emref->change_indicator = GTK_LABEL(indicator);
	gtk_box_append(GTK_BOX(emblem_container), indicator);

	#undef indicator
	#define emblem_cell _widget_placeholder_2_

	emblem_cell = g_object_new(
		GNUI_TYPE_EMBLEM_PICKER_EMBLEM,
		"child", emblem_container,
		NULL
	);

	gnui_set_emblem_cell_gtk_state(emblem_cell, emref->current_state);
	gtk_widget_add_css_class(emblem_cell, "unsupported");
	GNUI_EMBLEM_PICKER_EMBLEM(emblem_cell)->emref = emref;
	emref->controller_cell = emblem_cell;

	gtk_flow_box_insert(
		GTK_FLOW_BOX(priv->pages[PAGE_UNSUPPORTED].emblem_view),
		emblem_cell,
		-1
	);

	#undef emblem_cell
	#undef emblem_container

}


/**

    gnui_emblem_picker_add_standard_emblems:
    @self:      (not nullable): The emblem picker
    @priv:      (not nullable): The emblem picker's private `struct`

    Add XDG standard emblems to the emblem picker

    Returns:    `true` if all standard emblems are supported by the current
                theme, `false` otherwise

**/
static bool gnui_emblem_picker_add_standard_emblems (
	GnuiEmblemPicker * const self,
	GnuiEmblemPickerPrivate * const priv
) {

	const GList * refllnk;
	EmblemReference * emref;
	bool has_one_page = true;
	gsize idx = 0;

	do {

		if (
			self->forbidden_emblems && g_strv_contains(
				(const gchar * const *) self->forbidden_emblems,
				xdg_emblems[idx]
			)
		) {

			continue;

		}

		refllnk = priv->references;

		search_standard_in_stock: if (refllnk) {

			if (
				strcmp(
					((EmblemReference *) refllnk->data)->name,
					xdg_emblems[idx])
			) {

				refllnk = refllnk->next;
				goto search_standard_in_stock;

			}

			continue;

		}

		emref = g_new(EmblemReference, 1);
		emref->name = g_strdup(xdg_emblems[idx]);
		emref->unsupported = true;
		emref->inconsistent_group = NULL;
		emref->saved_state = emref->current_state = GNUI_EMBLEM_STATE_NORMAL;
		gnui_emblem_picker_draw_unsupported_emblem(self, priv, emref);
		priv->references = g_list_prepend(priv->references, emref);
		has_one_page = false;

	} while (++idx < N_XDG_EMBLEMS);

	return has_one_page;

}


/**

    gnui_emblem_picker_repage_view:
    @self:      (not nullable): The emblem picker
    @priv:      (not nullable): The emblem picker's private `struct`

    Repage the emblem picker

**/
static void gnui_emblem_picker_repage_view (
	const GnuiEmblemPicker * const self,
	GnuiEmblemPickerPrivate * const priv
) {

	if (!priv->is_single_page && priv->current_view == priv->single_page) {

		g_object_ref(priv->single_page);
		gtk_widget_unparent(priv->single_page);
		g_object_ref(priv->pages[PAGE_SUPPORTED].search_box);

		gtk_box_remove(
			GTK_BOX(priv->single_page),
			priv->pages[PAGE_SUPPORTED].search_box
		);

		gtk_box_prepend(
			GTK_BOX(priv->page_1_of_2),
			priv->pages[PAGE_SUPPORTED].search_box
		);

		g_object_unref(priv->pages[PAGE_SUPPORTED].search_box);
		g_object_ref(priv->supported_container);
		gtk_box_remove(GTK_BOX(priv->single_page), priv->supported_container);
		gtk_box_prepend(GTK_BOX(priv->page_1_of_2), priv->supported_container);
		g_object_unref(priv->supported_container);
		gtk_widget_set_parent(priv->two_pages, GTK_WIDGET(self));
		g_object_unref(priv->two_pages);
		priv->current_view = priv->two_pages;

	} else if (priv->is_single_page && priv->current_view == priv->two_pages) {

		g_object_ref(priv->two_pages);
		gtk_widget_unparent(priv->two_pages);
		g_object_ref(priv->pages[PAGE_SUPPORTED].search_box);

		gtk_box_remove(
			GTK_BOX(priv->page_1_of_2),
			priv->pages[PAGE_SUPPORTED].search_box
		);

		gtk_box_prepend(
			GTK_BOX(priv->single_page),
			priv->pages[PAGE_SUPPORTED].search_box
		);

		g_object_unref(priv->pages[PAGE_SUPPORTED].search_box);
		g_object_ref(priv->supported_container);
		gtk_box_remove(GTK_BOX(priv->page_1_of_2), priv->supported_container);
		gtk_box_prepend(GTK_BOX(priv->single_page), priv->supported_container);
		g_object_unref(priv->supported_container);
		gtk_widget_set_parent(priv->single_page, GTK_WIDGET(self));
		g_object_unref(priv->single_page);
		priv->current_view = priv->single_page;

	}

}


/**

    gnui_emblem_picker_refresh_icons:
    @self:      (not nullable): The emblem picker
    @priv:      (not nullable): The emblem picker's private `struct`

    Refresh the icons displayed by an emblem picker

**/
static void gnui_emblem_picker_refresh_icons (
	GnuiEmblemPicker * const self,
	GnuiEmblemPickerPrivate * const priv
) {

	g_return_if_fail(priv->current_view != NULL);

	GList
		* new_stack = get_available_emblems(self, priv),
		* old_refs = g_steal_pointer(&priv->references),
		* refllnk = old_refs,
		* namellnk;

	EmblemReference * emref;
	bool has_one_page = true;

	review_old_stack: if (refllnk) {

		emref = refllnk->data;

		/*  Old stack has a forbidden emblem - delete it  */

		if (
			self->forbidden_emblems && g_strv_contains(
				(const gchar * const *) self->forbidden_emblems,
				emref->name
			)
		) {

			refllnk = refllnk->next;
			goto review_old_stack;

		}

		namellnk = new_stack;

		find_supported_emblem: if (namellnk) {

			if (strcmp(emref->name, namellnk->data)) {

				namellnk = namellnk->next;
				goto find_supported_emblem;

			}

			/*  Both stacks (new and old) share this emblem  */

			g_free(namellnk->data);
			new_stack = g_list_delete_link(new_stack, namellnk);

			if (emref->unsupported) {

				/*  New stack supports the emblem: make it visual  */

				gtk_flow_box_remove(
					GTK_FLOW_BOX(priv->pages[PAGE_UNSUPPORTED].emblem_view),
					emref->controller_cell
				);

				emref->unsupported = false;
				gnui_emblem_picker_draw_supported_emblem(self, priv, emref);

			}

			goto add_and_continue;

		}

		if (
			emref->saved_state == GNUI_EMBLEM_STATE_NORMAL &&
			emref->current_state == GNUI_EMBLEM_STATE_NORMAL
		) {

			/*  New stack doesn't support the untoggled emblem: delete it  */

			refllnk = refllnk->next;
			goto review_old_stack;

		}

		if (!emref->unsupported) {

			/*  New stack doesn't support the toggled emblem: textualize it  */

			gtk_flow_box_remove(
				GTK_FLOW_BOX(priv->pages[PAGE_SUPPORTED].emblem_view),
				emref->controller_cell
			);

			emref->unsupported = true;
			gnui_emblem_picker_draw_unsupported_emblem(self, priv, emref);

		}

		has_one_page = false;


		/* \                                  /\
		\ */     add_and_continue:           /* \
		 \/     ________________________     \ */


		priv->references = gnui_list_prepend_llink(
			priv->references,
			gnui_list_u_detach_and_move_to_next(&old_refs, &refllnk)
		);

		goto review_old_stack;

	}

	/*  Add the remaining novel emblems  */

	namellnk = new_stack;

	while (namellnk) {

		emref = g_new(EmblemReference, 1);
		emref->name = namellnk->data;
		emref->unsupported = false;
		emref->inconsistent_group = NULL;
		emref->saved_state = emref->current_state = GNUI_EMBLEM_STATE_NORMAL;
		gnui_emblem_picker_draw_supported_emblem(self, priv, emref);
		refllnk = namellnk;
		namellnk = namellnk->next;
		refllnk->data = emref;
		priv->references = gnui_list_prepend_llink(priv->references, refllnk);

	}

	if (self->ensure_standard) {

		has_one_page &= gnui_emblem_picker_add_standard_emblems(self, priv);

	}

	priv->is_single_page = has_one_page;

	for (
		refllnk = old_refs;
			refllnk;
		gnui_emblem_picker_reference_destroy(priv, refllnk->data),
		refllnk = refllnk->next
	);

	g_list_free(old_refs);

	priv->references = g_list_sort(
		priv->references,
		(GCompareFunc) gnui_emblem_picker_sort_emblem_references
	);

	refllnk = priv->references;
	gnui_emblem_picker_repage_view(self, priv);

}


/*  Event listeners  */


/**

    gnui_emblem_picker__on_emblem_toggle:
    @emblem_view:   (auto) (unused): The `GtkFlowBox` child
    @emblem_widget: (auto) (not nullable): The `GnuiEmblemPickerEmblem` GObject
                    passed as `GtkFlowBoxChild`
    @v_self:        (auto) (not nullable): The emblem picker passed as
                    `gpointer`

    Event handler for the #GtkFlowBox::child-activated event of the emblem
    picker's `GtkFlowBox`

**/
static void gnui_emblem_picker__on_emblem_toggle (
	G_GNUC_UNUSED GtkFlowBox * const emblem_view,
	GtkFlowBoxChild * const emblem_widget,
	const gpointer v_self
) {

	gnui_emblem_picker_toggle_emblem_reference(
		v_self,
		GNUI_EMBLEM_PICKER_EMBLEM(emblem_widget)->emref
	);

}


/**

    gnui_emblem_picker__on_navigate_click:
    @nav_button:    (auto) (unused): Navigation `GtkButton`
    @v_target_page: (auto) (not nullable): The `GnuiEmblemPickerPage` of
                    destination passed as `gpointer`

    Event handler for the #GtkButton::clicked event of the emblem picker's
    navigation buttons

**/
static void gnui_emblem_picker__on_navigate_click (
	GtkButton * const nav_button G_GNUC_UNUSED,
	const gpointer v_target_page
) {

	#define target_page ((const GnuiEmblemPickerPage *) v_target_page)

	adw_leaflet_navigate(target_page->pager, target_page->way_in);

	GtkFlowBoxChild * const first_emblem = gtk_flow_box_get_child_at_index(
		GTK_FLOW_BOX(target_page->emblem_view),
		0
	);

	if (first_emblem) {

		gtk_widget_grab_focus(GTK_WIDGET(first_emblem));


	}

	#undef target_page

}


/**

    gnui_emblem_picker__on_icon_theme_change:
    @theme:         (auto) (not nullable): The current display's icon theme
    @v_self:        (auto) (not nullable): The emblem picker passed as
                    `gpointer`

    Event handler for the #GtkIconTheme::changed event of the icon theme

**/
static void gnui_emblem_picker__on_icon_theme_change (
	GtkIconTheme * const theme,
	const gpointer v_self
) {

	GnuiEmblemPickerPrivate * const priv =
		gnui_emblem_picker_get_instance_private(v_self);

	priv->icon_theme = theme;
	gnui_emblem_picker_refresh_icons(v_self, priv);

}


/**

    gnui_emblem_picker__on_search_change:
    @entry:     (auto) (not nullable): The `GtkSearchEntry`
    @v_page:    (auto) (not nullable): The current `GnuiEmblemPickerPage`
                passed as `gpointer`

    Event handler for the #GtkSearchEntry::search-changed event of the emblem
    view's searchbox

**/
static void gnui_emblem_picker__on_search_change (
	GtkSearchEntry * const entry,
	const gpointer v_page
) {

	#define page ((const GnuiEmblemPickerPage *) v_page)

	const gchar * const query = gtk_editable_get_text(GTK_EDITABLE(entry));
	const gboolean has_text = query && *query;

	gtk_revealer_set_reveal_child(GTK_REVEALER(page->search_box), has_text);

	gtk_widget_grab_focus(
		has_text ?
			GTK_WIDGET(entry)
		:
			gtk_search_entry_get_key_capture_widget(entry)
	);

	gtk_flow_box_invalidate_filter(GTK_FLOW_BOX(page->emblem_view));

	#undef page

}


/**

    gnui_emblem_picker__on_search_stop:
    @entry:     (auto) (not nullable): The `GtkSearchEntry`
    @v_page:    (auto) (not nullable): The current `GnuiEmblemPickerPage`
                passed as `gpointer`

    Event handler for the #GtkSearchEntry::stop-search event of the emblem
    view's searchbox

**/
static void gnui_emblem_picker__on_search_stop (
	GtkSearchEntry * entry,
	const gpointer v_page
) {

	#define page ((const GnuiEmblemPickerPage *) v_page)

	gtk_revealer_set_reveal_child(GTK_REVEALER(page->search_box), false);
	gtk_flow_box_invalidate_filter(GTK_FLOW_BOX(page->emblem_view));
	gtk_widget_grab_focus(gtk_search_entry_get_key_capture_widget(entry));

	#undef page

}


/*  Implementation's core  */


/**

    gnui_emblem_picker_load_emblems:
    @self:      (not nullable): The emblem picker
    @priv:      (not nullable): The emblem picker's private `struct`

    Populate the emblem picker's `GtkFlowBox` widgets

**/
static void gnui_emblem_picker_load_emblems (
	GnuiEmblemPicker * const self,
	GnuiEmblemPickerPrivate * const priv
) {

	typedef struct _AssignedEmblem {
		gchar * emblem_name;
		GList * files;
	} AssignedEmblem;

	GFileInfo * finfo;
	GError * readerr = NULL;
	gchar * uri;
	gchar ** file_emblems;
	AssignedEmblem * owned_emblem;
	EmblemReference * emref;
	GList * assignments = NULL, * assignllnk, * _list_placeholder_1_;
	gsize file_amount = 0;

	#define filellnk _list_placeholder_1_

	for (filellnk = self->mapped_files; filellnk; filellnk = filellnk->next) {

		file_amount++;

		finfo = g_file_query_info(
			filellnk->data,
			G_FILE_ATTRIBUTE_METADATA_EMBLEMS,
			G_FILE_QUERY_INFO_NONE,
			NULL,
			&readerr
		);

		if (!finfo) {

			uri = g_file_get_uri(filellnk->data);

			g_warning(
				"%s (%s) // %s",
				_("Could not read file's emblems"),
				uri ? uri : _("unknown location"),
				readerr->message
			);

			g_free(uri);
			g_clear_error(&readerr);
			continue;

		}

		if (
			(file_emblems = g_file_info_get_attribute_stringv(
				finfo,
				G_FILE_ATTRIBUTE_METADATA_EMBLEMS
			))
		) {

			while (*file_emblems) {

				assignllnk = assignments;

				find_file_emblem: if (assignllnk) {

					owned_emblem = assignllnk->data;

					if (strcmp(owned_emblem->emblem_name, *file_emblems)) {

						assignllnk = assignllnk->next;
						goto find_file_emblem;

					}

					owned_emblem->files = g_list_prepend(
						owned_emblem->files,
						filellnk->data
					);

				} else {

					owned_emblem = g_new(AssignedEmblem, 1);
					owned_emblem->emblem_name = g_strdup(*file_emblems);
					owned_emblem->files = g_list_prepend(NULL, filellnk->data);
					assignments = g_list_prepend(assignments, owned_emblem);

				}

				file_emblems++;

			}

		}

		g_object_unref(finfo);

	}

	#undef filellnk
	#define namellnk _list_placeholder_1_

	if ((namellnk = get_available_emblems(self, priv))) {

		priv->references = namellnk;


		/* \                                  /\
		\ */     add_supported_emblem:       /* \
		 \/     ________________________     \ */


		emref = g_new(EmblemReference, 1);
		emref->unsupported = false;
		emref->name = namellnk->data;
		emref->inconsistent_group = NULL;
		assignllnk = assignments;

		find_owned_emblem: if (assignllnk) {

			owned_emblem = assignllnk->data;

			if (strcmp(namellnk->data, owned_emblem->emblem_name)) {

				assignllnk = assignllnk->next;
				goto find_owned_emblem;

			}

			emref->inconsistent_group = owned_emblem->files;
			g_free(owned_emblem->emblem_name);
			g_free(owned_emblem);
			assignments = g_list_delete_link(assignments, assignllnk);

		}

		if (
			emref->inconsistent_group &&
			g_list_length(emref->inconsistent_group) < file_amount
		) {

			emref->saved_state = emref->current_state =
				GNUI_EMBLEM_STATE_INCONSISTENT;

		} else {

			emref->saved_state = emref->current_state =
				emref->inconsistent_group ?
					GNUI_EMBLEM_STATE_SELECTED
				:
					GNUI_EMBLEM_STATE_NORMAL;

			g_clear_pointer(&emref->inconsistent_group, g_list_free);

		}

		gnui_emblem_picker_draw_supported_emblem(self, priv, emref);
		namellnk->data = emref;

		if (namellnk->next) {

			namellnk = namellnk->next;
			goto add_supported_emblem;

		}

		if (assignments) {

			namellnk->next = assignments;
			assignments->prev = namellnk;

		}

	} else {

		priv->references = assignments;

	}

	#undef namellnk

	assignllnk = assignments;

	while (assignllnk) {

		owned_emblem = assignllnk->data;

		if (
			self->forbidden_emblems && g_strv_contains(
				(const gchar * const *) self->forbidden_emblems,
				owned_emblem->emblem_name
			)
		) {

			g_free(owned_emblem->emblem_name);
			g_list_free(owned_emblem->files);
			g_free(owned_emblem);
			GNUI_LIST_DELETE_AND_MOVE_TO_NEXT(&assignments, &assignllnk);
			continue;

		}

		emref = g_new(EmblemReference, 1);
		emref->unsupported = true;
		emref->name = owned_emblem->emblem_name;
		emref->inconsistent_group = owned_emblem->files;

		if (
			g_list_length(emref->inconsistent_group) < file_amount
		) {

			emref->saved_state = emref->current_state =
				GNUI_EMBLEM_STATE_INCONSISTENT;

		} else {

			emref->saved_state = emref->current_state =
				GNUI_EMBLEM_STATE_SELECTED;

			g_clear_pointer(&emref->inconsistent_group, g_list_free);

		}

		gnui_emblem_picker_draw_unsupported_emblem(self, priv, emref);
		g_free(owned_emblem);
		assignllnk->data = emref;
		assignllnk = assignllnk->next;

	}

	priv->is_single_page =
		self->ensure_standard ?
			!assignments &&
			gnui_emblem_picker_add_standard_emblems(self, priv)
		:
			!assignments;

	priv->references = g_list_sort(
		priv->references,
		(GCompareFunc) gnui_emblem_picker_sort_emblem_references
	);

}


/**

    gnui_emblem_picker_restart_session:
    @self:      (not nullable): The emblem picker
    @priv:      (not nullable): The emblem picker's private `struct`

    Discard all selection changes and start a new session

**/
static void gnui_emblem_picker_restart_session (
	GnuiEmblemPicker * const self,
	GnuiEmblemPickerPrivate * const priv
) {

	const EmblemReference * emref;

	for (const GList * llnk = priv->references; llnk; llnk = llnk->next) {

		emref = (EmblemReference *) llnk->data;

		gtk_flow_box_remove(
			GTK_FLOW_BOX(priv->pages[emref->unsupported].emblem_view),
			emref->controller_cell
		);

		g_free(emref->name);
		g_list_free(emref->inconsistent_group);

	}

	g_list_free_full(priv->references, g_free);
	priv->references = NULL;
	gnui_emblem_picker_load_emblems(self, priv);
	gnui_emblem_picker_repage_view(self, priv);

}


/**

    gnui_emblem_picker_build_action_button:
    @title:     (not nullable): The button's title
    @subtitle:  (not nullable): The button's subtitle
    @prefix:    (not nullable): The button's prefix widget
    @suffix:    (not nullable): The button's suffix widget
    @callback:  (not nullable): The button's #GtkButton::clicked handler
    @data:      (not nullable): The custom data passed to @p callback

    Create an action button widget for the emblem picker

    Returns:    The action button created

**/
static GtkWidget * gnui_emblem_picker_build_action_button (
	const gchar * const title,
	const gchar * const subtitle,
	GtkWidget * const prefix,
	GtkWidget * const suffix,
	void (* const callback) (GtkButton * button, gpointer data),
	const gpointer data
) {

	GtkWidget * _widget_placeholder_1_, * _widget_placeholder_2_;

	#define header _widget_placeholder_1_
	#define label _widget_placeholder_2_

	header = g_object_new(
		GTK_TYPE_BOX,
		"orientation", GTK_ORIENTATION_VERTICAL,
		"spacing", 0,
		"hexpand", true,
		NULL
	);

	gtk_widget_add_css_class(header, "header");

	label = g_object_new(
		GTK_TYPE_LABEL,
		"label", title,
		"halign", GTK_ALIGN_START,
		NULL
	);

	gtk_widget_add_css_class(label, "title");
	gtk_box_append(GTK_BOX(header), label);

	#undef label
	#define sublabel _widget_placeholder_2_

	sublabel = g_object_new(
		GTK_TYPE_LABEL,
		"label", subtitle,
		"halign", GTK_ALIGN_START,
		NULL
	);

	gtk_widget_add_css_class(sublabel, "subtitle");
	gtk_box_append(GTK_BOX(header), sublabel);

	#undef sublabel
	#define content _widget_placeholder_2_

	content = g_object_new(
		GTK_TYPE_BOX,
		"orientation", GTK_ORIENTATION_HORIZONTAL,
		"spacing", 12,
		"hexpand", true,
		NULL
	);

	gtk_box_append(GTK_BOX(content), prefix);
	gtk_box_append(GTK_BOX(content), header);
	gtk_box_append(GTK_BOX(content), suffix);

	#undef header
	#define button _widget_placeholder_1_

	button = g_object_new(GTK_TYPE_BUTTON, "child", content, NULL);
	gtk_widget_add_css_class(button, "action");
	gtk_widget_add_css_class(button, "card");
	g_signal_connect(button, "clicked", G_CALLBACK(callback), data);
	return button;

	#undef button

}


/*  Class functions  */


/**

    gnui_emblem_picker_dispose:
    @object:    (auto) (not nullable): The emblem picker passed as `GObject`

    Class handler for the #Object.dispose() method on the emblem picker
    instance

**/
static void gnui_emblem_picker_dispose (
    GObject * const object
) {

	GnuiEmblemPicker * const self = GNUI_EMBLEM_PICKER(object);

	GnuiEmblemPickerPrivate * const priv =
		gnui_emblem_picker_get_instance_private(self);

	g_clear_signal_handler(&priv->icon_theme_refresh_signal, priv->icon_theme);

	if (priv->current_view) {

		g_object_unref(
			priv->current_view == priv->single_page ?
				priv->two_pages
			:
				priv->single_page
		);

	}

	g_clear_pointer(&priv->current_view, gtk_widget_unparent);

	for (const GList * llnk = priv->references; llnk; llnk = llnk->next) {

		g_free(((EmblemReference *) llnk->data)->name);
		g_list_free(((EmblemReference *) llnk->data)->inconsistent_group);

	}

	g_list_free(priv->references);
	g_list_free_full(self->mapped_files, g_object_unref);
	g_strfreev(self->forbidden_emblems);
	G_OBJECT_CLASS(gnui_emblem_picker_parent_class)->dispose(object);

}


/**

    gnui_emblem_picker_get_property:
    @object:    (auto) (not nullable): The emblem picker passed as `GObject`
    @prop_id:   (auto): The id of the property to retrieve
    @value:     (auto) (out): The `GValue` that must be returned
    @pspec:     (auto) (not nullable): The `GParamSpec` of the property

    Class handler for `g_object_get()` on the emblem picker instance

**/
static void gnui_emblem_picker_get_property (
	GObject * const object,
	const guint prop_id,
	GValue * const value,
	GParamSpec * const pspec
) {

	#define self GNUI_EMBLEM_PICKER(object)

	switch (prop_id) {

		case PROPERTY_MAPPED_FILES:

			/*  Transfer full  */

			g_value_set_pointer(value, self->mapped_files);
			break;

		case PROPERTY_FORBIDDEN_EMBLEMS:

			/*  Transfer full  */

			g_value_set_static_boxed(
				value,
				self->forbidden_emblems
			);

			break;

		case PROPERTY_ENSURE_STANDARD:

			g_value_set_boolean(value, self->ensure_standard);
			break;

		case PROPERTY_MODIFIED:

			g_value_set_boolean(value, self->modified);
			break;

		case PROPERTY_REVEAL_CHANGES:

			g_value_set_boolean(value, self->reveal_changes);
			break;

		default:

			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);

	}

	#undef self

}


/**

    gnui_emblem_picker_set_property:
    @object:    (auto) (not nullable): The emblem picker passed as `GObject`
    @prop_id:   (auto): The id of the property to set
    @value:     (auto): The `GValue` containing the new value assigned to the
                property
    @pspec:     (auto) (not nullable): The `GParamSpec` of the property

    Class handler for `g_object_set()` on the emblem picker instance

**/
static void gnui_emblem_picker_set_property (
	GObject * const object,
	const guint prop_id,
	const GValue * const value,
	GParamSpec * const pspec
) {

	GnuiEmblemPicker * const self = GNUI_EMBLEM_PICKER(object);

	GnuiEmblemPickerPrivate * const priv =
		gnui_emblem_picker_get_instance_private(self);

	union {
		bool b;
		const gchar * const * a;
		GList * l;
	} val;

	switch (prop_id) {

		case PROPERTY_MAPPED_FILES:

			/*  Transfer none  */

			val.l = g_value_get_pointer(value);

			if (
				self->mapped_files ?
					gnui_lists_are_equal(val.l, self->mapped_files)
				:
					!val.l
			) {

				return;

			}

			self->mapped_files = g_list_copy(val.l);

			for (GList * llnk = self->mapped_files; llnk; llnk = llnk->next) {

				g_object_ref(llnk->data);

			}

			if (priv->current_view) {

				/*  The property is being set after construction  */

				gnui_emblem_picker_restart_session(self, priv);

			}

			break;

		case PROPERTY_FORBIDDEN_EMBLEMS:

			/*  Transfer none  */

			val.a = g_value_get_boxed(value);

			if (
				(
					!val.a && !self->forbidden_emblems
				) || (
					val.a &&
					self->forbidden_emblems &&
					g_strv_equal(
						(const gchar * const *) self->forbidden_emblems,
						val.a
					)
				)
			) {

				return;

			}

			g_strfreev(self->forbidden_emblems);
			self->forbidden_emblems = g_value_dup_boxed(value);

			if (priv->current_view) {

				/*  The property is being set after construction  */

				gnui_emblem_picker_refresh_icons(self, priv);

			}

			break;

		case PROPERTY_ENSURE_STANDARD:

			if (
				self->ensure_standard == (val.b = g_value_get_boolean(value))
			) {

				return;

			}

			self->ensure_standard = val.b;

			if (priv->current_view) {

				/*  The property is being changed after construction  */

				if (val.b) {

					priv->is_single_page &=
						gnui_emblem_picker_add_standard_emblems(self, priv);

					gnui_emblem_picker_repage_view(self, priv);

				} else {

					gnui_emblem_picker_refresh_icons(self, priv);

				}

			}

			break;

		case PROPERTY_MODIFIED:

			if (self->modified == (val.b = g_value_get_boolean(value))) {

				return;

			}

			self->modified = val.b;

			if (priv->current_view) {

				/*  The property is being set after construction  */

				g_signal_emit(
					self,
					signals[SIGNAL_MODIFIED_CHANGED],
					0,
					val.b
				);

			}

			break;

		case PROPERTY_REVEAL_CHANGES:

			if (self->reveal_changes == (val.b = g_value_get_boolean(value))) {

				return;

			}

			self->reveal_changes = val.b;
			break;

		default:

			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);

	}

	g_object_notify_by_pspec(G_OBJECT(self), props[prop_id]);

}


/**

    gnui_emblem_picker_constructed:
    @object:    (auto) (not nullable): The emblem picker passed as `GObject`

    Class handler for the #Object.constructed() method on the emblem picker
    instance

**/
static void gnui_emblem_picker_constructed (
	GObject * object
) {

	G_OBJECT_CLASS(gnui_emblem_picker_parent_class)->constructed(object);

	GnuiEmblemPicker * const self = GNUI_EMBLEM_PICKER(object);

	GnuiEmblemPickerPrivate * const priv =
		gnui_emblem_picker_get_instance_private(self);

	gnui_emblem_picker_load_emblems(self, priv);

	if (priv->is_single_page) {

		gtk_box_prepend(
			GTK_BOX(priv->single_page),
			priv->pages[PAGE_SUPPORTED].search_box
		);

		gtk_box_prepend(GTK_BOX(priv->single_page), priv->supported_container);
		gtk_widget_set_parent(priv->single_page, GTK_WIDGET(self));
		priv->current_view = priv->single_page;
		g_object_ref_sink(priv->two_pages);

	} else {

		gtk_box_prepend(
			GTK_BOX(priv->page_1_of_2),
			priv->pages[PAGE_SUPPORTED].search_box
		);

		gtk_box_prepend(GTK_BOX(priv->page_1_of_2), priv->supported_container);
		gtk_widget_set_parent(priv->two_pages, GTK_WIDGET(self));
		priv->current_view = priv->two_pages;
		g_object_ref_sink(priv->single_page);

	}

}


/**

    gnui_cclosure_marshal_VOID__STRING_ENUM_ENUM_POINTER:

    Generated by `glib-genmarshal` with `VOID:STRING,ENUM,ENUM,POINTER`

**/
static void gnui_cclosure_marshal_VOID__STRING_ENUM_ENUM_POINTER (
	GClosure * const closure,
	GValue * const return_value G_GNUC_UNUSED,
	const guint n_param_values,
	const GValue * const param_values,
	const gpointer invocation_hint G_GNUC_UNUSED,
	const gpointer marsh_data
) {

	#ifdef G_ENABLE_DEBUG
	#define g_marshal_value_peek_enum(v)     g_value_get_enum (v)
	#define g_marshal_value_peek_string(v)   (char*) g_value_get_string (v)
	#define g_marshal_value_peek_pointer(v)  g_value_get_pointer (v)
	#else /* !G_ENABLE_DEBUG */
	/* WARNING: This code accesses GValues directly, which is UNSUPPORTED API.
	 *          Do not access GValues directly in your code. Instead, use the
	 *          g_value_get_*() functions
	 */
	#define g_marshal_value_peek_enum(v)     (v)->data[0].v_long
	#define g_marshal_value_peek_string(v)   (v)->data[0].v_pointer
	#define g_marshal_value_peek_pointer(v)  (v)->data[0].v_pointer
	#endif /* !G_ENABLE_DEBUG */

	g_return_if_fail(n_param_values == 5);

	typedef void (* GMarshalFunc_VOID__STRING_ENUM_ENUM_POINTER) (
		gpointer data1,
		gpointer arg1,
		gint arg2,
		gint arg3,
		gpointer arg4,
		gpointer data2
	);

	gpointer data1, data2;

	if (G_CCLOSURE_SWAP_DATA(closure)) {

		data1 = closure->data;
		data2 = g_value_peek_pointer(param_values + 0);

	} else {

		data1 = g_value_peek_pointer(param_values + 0);
		data2 = closure->data;

	}

	(
		*(
			(GMarshalFunc_VOID__STRING_ENUM_ENUM_POINTER *) (
				marsh_data ? &marsh_data : &((GCClosure *) closure)->callback
			)
		)
	)(
		data1,
		g_marshal_value_peek_string(param_values + 1),
		g_marshal_value_peek_enum(param_values + 2),
		g_marshal_value_peek_enum(param_values + 3),
		g_marshal_value_peek_pointer(param_values + 4),
		data2
	);

	#undef g_marshal_value_peek_pointer
	#undef g_marshal_value_peek_string
	#undef g_marshal_value_peek_enum

}


/**

    gnui_emblem_picker_class_init:
    @klass:     (auto) (not nullable): The `GObject` klass

    The init function of the emblem picker class

**/
static void gnui_emblem_picker_class_init (
	GnuiEmblemPickerClass * const klass
) {

	GObjectClass * const object_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass * const widget_class = GTK_WIDGET_CLASS(klass);

	object_class->get_property = gnui_emblem_picker_get_property;
	object_class->set_property = gnui_emblem_picker_set_property;
	object_class->constructed = gnui_emblem_picker_constructed;
	object_class->dispose = gnui_emblem_picker_dispose;

	/**

	    GnuiEmblemPicker:mapped-files: (nullable) (transfer none)

	**/
	props[PROPERTY_MAPPED_FILES] = g_param_spec_pointer(
		"mapped-files",
		"const GList *",
		"GList of GFile objects to map",
		G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY |
			G_PARAM_STATIC_STRINGS
	);

	/**

	    GnuiEmblemPicker:forbidden-emblems: (nullable) (transfer none)

	**/
	props[PROPERTY_FORBIDDEN_EMBLEMS] = g_param_spec_boxed(
		"forbidden-emblems",
		"const gchar * const *",
		"Array of strings containing the names of the emblems to ignore",
		G_TYPE_STRV,
		G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY |
			G_PARAM_STATIC_STRINGS
	);

	props[PROPERTY_ENSURE_STANDARD] = g_param_spec_boolean(
		"ensure-standard",
		"gboolean",
		"Whether to ensure XDG emblems, even when not supported by the "
			"current theme",
		false,
		G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY |
			G_PARAM_STATIC_STRINGS
	);

	props[PROPERTY_MODIFIED] = g_param_spec_boolean(
		"modified",
		"gboolean",
		"Whether the emblems have changed since last save",
		false,
		G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY |
			G_PARAM_STATIC_STRINGS
	);

	props[PROPERTY_REVEAL_CHANGES] = g_param_spec_boolean(
		"reveal-changes",
		"gboolean",
		"Whether changes since last save must be visible",
		false,
		G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY |
			G_PARAM_STATIC_STRINGS
	);

	g_object_class_install_properties(object_class, N_PROPERTIES, props);

	gtk_widget_class_set_layout_manager_type(
		widget_class,
		GTK_TYPE_BIN_LAYOUT
	);

	gtk_widget_class_set_css_name(widget_class, I_("emblempicker"));

	/**

	    GnuiEmblemPicker::modified-changed:
	    @self:      (auto) (non-nullable): The emblem picker that emitted the
	                signal
	    @modified:  (auto): The new value of the #GnuiEmblemPicker:modified
	                property

	    Signal emitted when the #GnuiEmblemPicker:modified property changes

	    #GnuiEmblemPickerSignalHandlerModifiedChanged is the function type of
	    reference for this signal, which takes parameters' constness into
	    account.

	**/
	signals[SIGNAL_MODIFIED_CHANGED] = g_signal_new(
		I_("modified-changed"),
		G_TYPE_FROM_CLASS(klass),
		G_SIGNAL_RUN_FIRST,
		0,
		NULL,
		NULL,
		g_cclosure_marshal_VOID__BOOLEAN,
		G_TYPE_NONE,
		1,
		/*  Maps `gboolean modified`  */
		G_TYPE_BOOLEAN
	);

	/**

	    GnuiEmblemPicker::emblem-selected:
	    @self:                  (auto) (non-nullable): The emblem picker that
	                            emitted the signal
	    @emblem_name:           (auto) (transfer none) (non-nullable): The
	                            emblem name (do not free or modify it)
	    @saved_state:           (auto): The last saved state of the emblem
	    @current_state:         (auto): The current state of the emblem
	    @inconsistent_group:    (auto) (transfer none) (nullable): The
	                            inconsistent group of files that can have the
	                            emblem assigned independently from the other
	                            files or `NULL`

	    Signal emitted when an emblem is selected/unselected

	    #GnuiEmblemPickerSignalHandlerEmblemSelected is the function type of
	    reference for this signal, which takes parameters' constness into
	    account.

	**/
	signals[SIGNAL_EMBLEM_SELECTED] = g_signal_new(
		I_("emblem-selected"),
		G_TYPE_FROM_CLASS(klass),
		G_SIGNAL_RUN_FIRST,
		0,
		NULL,
		NULL,
		gnui_cclosure_marshal_VOID__STRING_ENUM_ENUM_POINTER,
		G_TYPE_NONE,
		4,
		/*  Maps `const gchar * emblem_name`  */
		G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE,
		/*  Maps `gint saved_state`  */
		G_TYPE_INT,
		/*  Maps `gint current_state`  */
		G_TYPE_INT,
		/*  Maps `const GList * inconsistent_group`  */
		G_TYPE_POINTER | G_SIGNAL_TYPE_STATIC_SCOPE
	);

}


/**

    gnui_emblem_picker_init:
    @self:  (auto) (not nullable): The newly allocated emblem picker

    The init function of the emblem picker instance

**/
static void gnui_emblem_picker_init (
	GnuiEmblemPicker * const self
) {

	GnuiEmblemPickerPrivate * const priv =
		gnui_emblem_picker_get_instance_private(self);

	GtkWidget * page_2_of_2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);

	/*  `-DGNUI_EMBLEM_PICKER_BUILD_FLAG_MANUAL_ENVIRONMENT` erases this  */
	GNUI_MODULE_ENSURE_ENVIRONMENT

	priv->icon_theme = gtk_icon_theme_get_for_display(
		gtk_widget_get_display(GTK_WIDGET(self))
	);

	priv->icon_theme_refresh_signal = g_signal_connect(
		priv->icon_theme,
		"changed",
		G_CALLBACK(gnui_emblem_picker__on_icon_theme_change),
		self
	);

	priv->two_pages = g_object_new(
		ADW_TYPE_LEAFLET,
		"can-navigate-back", true,
		"can-navigate-forward", true,
		"can-unfold", false,
		"hexpand", true,
		"vexpand", true,
		"width-request", GNUI_EMBLEM_PICKER_PAGE_MIN_WIDTH,
		"height-request", GNUI_EMBLEM_PICKER_PAGE_MIN_HEIGHT,
		NULL
	);

	/*  Supported emblems page  */

	priv->pages[PAGE_SUPPORTED].pager = ADW_LEAFLET(priv->two_pages);

	priv->pages[PAGE_SUPPORTED].emblem_view = g_object_new(
		GTK_TYPE_FLOW_BOX,
		"selection-mode", GTK_SELECTION_NONE,
		"activate-on-single-click", true,
		"homogeneous", true,
		"vexpand", false,
		"focusable", true,
		"valign", GTK_ALIGN_START,
		NULL
	);

	gtk_flow_box_set_filter_func(
		GTK_FLOW_BOX(priv->pages[PAGE_SUPPORTED].emblem_view),
		gnui_emblem_picker_filter_emblem_cell,
		&priv->pages[PAGE_SUPPORTED],
		NULL
	);

	gtk_flow_box_set_sort_func(
		GTK_FLOW_BOX(priv->pages[PAGE_SUPPORTED].emblem_view),
		gnui_emblem_picker_sort_emblem_cells,
		NULL,
		NULL
	);

	g_signal_connect(
		priv->pages[PAGE_SUPPORTED].emblem_view,
		"child-activated",
		G_CALLBACK(gnui_emblem_picker__on_emblem_toggle),
		self
	);

	priv->pages[PAGE_SUPPORTED].search_entry = gtk_search_entry_new();

	gtk_search_entry_set_key_capture_widget(
		GTK_SEARCH_ENTRY(priv->pages[PAGE_SUPPORTED].search_entry),
		priv->pages[PAGE_SUPPORTED].emblem_view
	);

	g_signal_connect(
		priv->pages[PAGE_SUPPORTED].search_entry,
		"search-changed",
		G_CALLBACK(gnui_emblem_picker__on_search_change),
		&priv->pages[PAGE_SUPPORTED]
	);

	g_signal_connect(
		priv->pages[PAGE_SUPPORTED].search_entry,
		"stop-search",
		G_CALLBACK(gnui_emblem_picker__on_search_stop),
		&priv->pages[PAGE_SUPPORTED]
	);

	priv->pages[PAGE_SUPPORTED].search_box = g_object_new(
		GTK_TYPE_REVEALER,
		"child", priv->pages[PAGE_SUPPORTED].search_entry,
		"reveal-child", false,
		NULL
	);

	priv->pages[PAGE_SUPPORTED].way_in = ADW_NAVIGATION_DIRECTION_BACK;

	/*  Unspported emblems page  */

	priv->pages[PAGE_UNSUPPORTED].pager = ADW_LEAFLET(priv->two_pages);

	priv->pages[PAGE_UNSUPPORTED].emblem_view = g_object_new(
		GTK_TYPE_FLOW_BOX,
		"selection-mode", GTK_SELECTION_NONE,
		"activate-on-single-click", true,
		"max-children-per-line", 1,
		"homogeneous", true,
		"vexpand", false,
		"focusable", true,
		"valign", GTK_ALIGN_START,
		NULL
	);

	gtk_flow_box_set_filter_func(
		GTK_FLOW_BOX(priv->pages[PAGE_UNSUPPORTED].emblem_view),
		gnui_emblem_picker_filter_emblem_cell,
		&priv->pages[PAGE_UNSUPPORTED],
		NULL
	);

	gtk_flow_box_set_sort_func(
		GTK_FLOW_BOX(priv->pages[PAGE_UNSUPPORTED].emblem_view),
		gnui_emblem_picker_sort_emblem_cells,
		NULL,
		NULL
	);

	g_signal_connect(
		priv->pages[PAGE_UNSUPPORTED].emblem_view,
		"child-activated",
		G_CALLBACK(gnui_emblem_picker__on_emblem_toggle),
		self
	);

	priv->pages[PAGE_UNSUPPORTED].search_entry = gtk_search_entry_new();

	gtk_search_entry_set_key_capture_widget(
		GTK_SEARCH_ENTRY(priv->pages[PAGE_UNSUPPORTED].search_entry),
		priv->pages[PAGE_UNSUPPORTED].emblem_view
	);

	g_signal_connect(
		priv->pages[PAGE_UNSUPPORTED].search_entry,
		"search-changed",
		G_CALLBACK(gnui_emblem_picker__on_search_change),
		&priv->pages[PAGE_UNSUPPORTED]
	);

	g_signal_connect(
		priv->pages[PAGE_UNSUPPORTED].search_entry,
		"stop-search",
		G_CALLBACK(gnui_emblem_picker__on_search_stop),
		&priv->pages[PAGE_UNSUPPORTED]
	);

	priv->pages[PAGE_UNSUPPORTED].search_box = g_object_new(
		GTK_TYPE_REVEALER,
		"child", priv->pages[PAGE_UNSUPPORTED].search_entry,
		"reveal-child", false,
		NULL
	);

	priv->pages[PAGE_UNSUPPORTED].way_in = ADW_NAVIGATION_DIRECTION_FORWARD;

	priv->page_1_of_2 = g_object_new(
		GTK_TYPE_BOX,
		"orientation", GTK_ORIENTATION_VERTICAL,
		"spacing", 0,
		"hexpand", true,
		"vexpand", true,
		NULL
	);

	priv->single_page = g_object_new(
		GTK_TYPE_BOX,
		"orientation", GTK_ORIENTATION_VERTICAL,
		"spacing", 0,
		"hexpand", true,
		"vexpand", true,
		"height-request", GNUI_EMBLEM_PICKER_PAGE_MIN_HEIGHT,
		"width-request", GNUI_EMBLEM_PICKER_PAGE_MIN_WIDTH,
		NULL
	);

	/*  Widget trees  */

	gtk_box_append(
		GTK_BOX(priv->page_1_of_2),
		gnui_emblem_picker_build_action_button(
			_("Unsupported emblems"),
			/*  TRANSLATORS: Please keep this text below 30 characters  */
			_("We coudn't render those :-("),
			gtk_image_new_from_icon_name("gnuisance.warning"),
			gtk_image_new_from_icon_name("gnuisance.nav-next"),
			gnui_emblem_picker__on_navigate_click,
			&priv->pages[PAGE_UNSUPPORTED]
		)
	);

	adw_leaflet_page_set_name(
		adw_leaflet_append(ADW_LEAFLET(priv->two_pages), priv->page_1_of_2),
		"supported-emblems"
	);

	gtk_box_append(
		GTK_BOX(page_2_of_2),
		gnui_emblem_picker_build_action_button(
			_("Unsupported emblems"),
			/*  TRANSLATORS: Please keep this text below 30 characters  */
			_("We coudn't render those :-("),
			gtk_image_new_from_icon_name("gnuisance.warning"),
			gtk_image_new_from_icon_name("gnuisance.nav-previous"),
			gnui_emblem_picker__on_navigate_click,
			&priv->pages[PAGE_SUPPORTED]
		)
	);

	gtk_box_append(
		GTK_BOX(page_2_of_2),
		g_object_new(
			GTK_TYPE_SCROLLED_WINDOW,
			"vexpand", true,
			"child", priv->pages[PAGE_UNSUPPORTED].emblem_view,
			NULL
		)
	);

	gtk_box_append(
		GTK_BOX(page_2_of_2),
		priv->pages[PAGE_UNSUPPORTED].search_box
	);

	adw_leaflet_page_set_name(
		adw_leaflet_append(ADW_LEAFLET(priv->two_pages), page_2_of_2),
		"unsupported-emblems"
	);

	priv->supported_container = g_object_new(
		GTK_TYPE_SCROLLED_WINDOW,
		"vexpand", true,
		"child", priv->pages[PAGE_SUPPORTED].emblem_view,
		NULL
	);

}


/**

    gnui_emblem_picker_emblem_class_init:
    @klass:     (auto) (unused): The `GObject` klass

    The init function of the emblem picker emblem class

**/
static void gnui_emblem_picker_emblem_class_init (
	GnuiEmblemPickerEmblemClass * const klass
) {

	gtk_widget_class_set_css_name(GTK_WIDGET_CLASS(klass), I_("emblem"));

}


/**

    gnui_emblem_picker_emblem_init:
    @self:  (auto) (unused): The newly allocated emblem picker emblem

    The init function of the emblem picker emblem instance

**/
static void gnui_emblem_picker_emblem_init (
	GnuiEmblemPickerEmblem * const self G_GNUC_UNUSED
) {

}



/*\
|*|
|*| PUBLIC FUNCTIONS
|*|
|*| (See the public header for the documentation)
|*|
\*/


void gnui_emblem_picker_refresh_states (
	GnuiEmblemPicker * const self
) {

	g_return_if_fail(GNUI_IS_EMBLEM_PICKER(self));

	const GnuiEmblemPickerPrivate * const priv =
		gnui_emblem_picker_get_instance_private(self);

	for (const GList * llnk = priv->references; llnk; llnk = llnk->next) {

		gnui_emblem_picker_refresh_cell(llnk->data);

	}

}


gboolean gnui_emblem_picker_save (
	GnuiEmblemPicker * const self,
	const GnuiEmblemPickerForeachSavedFileFunc for_each_file,
	const GnuiEmblemPickerSaveFlags flags,
	GCancellable * const cancellable,
	GError ** const error,
	const gpointer for_each_data
) {

	typedef struct _FileEmblems {
		GFile * location;
		GList * current;
		GList * saved;
		gsize curlen;
	} FileEmblems;

	g_return_val_if_fail(GNUI_IS_EMBLEM_PICKER(self), false);

	if (!self->mapped_files) {

		g_warning(
			_(
				"gnui_emblem_picker_save() - No files were passed to "
				"GnuiEmblemPicker widget with address %p."
			),
			(gpointer) self
		);

		return false;

	}

	const GnuiEmblemPickerPrivate * const priv =
		gnui_emblem_picker_get_instance_private(self);

	const GList * _cllink_1_;
	GList * tasks = NULL,  * _list_placeholder_2_, * _list_placeholder_3_;
	FileEmblems * task;
	EmblemReference * emref;
	GError * saverr = NULL;
	const gchar ** emblems_for_file, ** additions, ** removals;
	gsize _size_1_,  _size_2_;
	GFileAttributeType attribute_type;
	GnuiEmblemPickerSaveResult result;
	GnuiEmblemPickerSaveFlags fmatch;

	#define filellnk _cllink_1_

	for (filellnk = self->mapped_files; filellnk; filellnk = filellnk->next) {

		#define refllnk _list_placeholder_2_

		refllnk = priv->references;
		task = g_new(FileEmblems, 1);
		task->location = filellnk->data;
		task->current = NULL;
		task->saved = NULL;
		task->curlen = 0;


		/* \                                  /\
		\ */     process_reference:          /* \
		 \/     ________________________     \ */


		if (refllnk) {

			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wswitch"

			emref = refllnk->data;

			switch (emref->current_state) {

				case GNUI_EMBLEM_STATE_INCONSISTENT:

					#define inconsllnk _list_placeholder_3_

					inconsllnk = emref->inconsistent_group;


					/* \                                  /\
					\ */     is_in_inconsistency:        /* \
					 \/     ________________________     \ */


					if (!inconsllnk) {

						break;

					}

					if (inconsllnk->data != filellnk->data) {

						inconsllnk = inconsllnk->next;
						goto is_in_inconsistency;

					}

					#undef inconsllnk

				/*  fallthrough  */

				case GNUI_EMBLEM_STATE_SELECTED:

					task->current = g_list_prepend(task->current, emref);
					task->curlen++;

			}

			switch (emref->saved_state) {

				case GNUI_EMBLEM_STATE_INCONSISTENT:

					#define inconsllnk _list_placeholder_3_

					inconsllnk = emref->inconsistent_group;


					/* \                                  /\
					\ */     was_in_inconsistency:       /* \
					 \/     ________________________     \ */


					if (!inconsllnk) {

						break;

					}

					if (inconsllnk->data != filellnk->data) {

						inconsllnk = inconsllnk->next;
						goto was_in_inconsistency;

					}

					#undef inconsllnk

				/*  fallthrough  */

				case GNUI_EMBLEM_STATE_SELECTED:

					task->saved = g_list_prepend(task->saved, emref);

			}

			#pragma GCC diagnostic pop

			refllnk = refllnk->next;
			goto process_reference;

		}

		#undef refllnk

		tasks = g_list_prepend(tasks, task);

	}

	#undef filellnk
	#define taskllnk _cllink_1_

	for (taskllnk = tasks; taskllnk; taskllnk = taskllnk->next) {

		task = taskllnk->data;

		if (task->current) {

			#define alllen _size_1_
			#define idx _size_2_
			#define crefllnk _list_placeholder_2_

			alllen = task->curlen; 
			attribute_type = G_FILE_ATTRIBUTE_TYPE_STRINGV;
			emblems_for_file = g_new(const gchar *, alllen + 1);

			for (
				idx = 0, crefllnk = task->current;
					crefllnk && idx < alllen;
				emblems_for_file[idx++] =
					((EmblemReference *) crefllnk->data)->name,
				crefllnk = crefllnk->next
			);

			emblems_for_file[idx] = NULL;

			#undef crefllnk
			#undef idx
			#undef alllen

		} else {

			attribute_type = G_FILE_ATTRIBUTE_TYPE_INVALID;
			emblems_for_file = NULL;

		}

		#define crefllnk _list_placeholder_2_
		#define orefllnk _list_placeholder_3_
		#define newlen _size_1_

		crefllnk = task->current;
		newlen = 0;


		/* \                                  /\
		\ */     process_current:            /* \
		 \/     ________________________     \ */


		if (crefllnk) {

			orefllnk = task->saved;


			/* \                                  /\
			\ */     process_saved:              /* \
			 \/     ________________________     \ */


			if (orefllnk) {

				if (crefllnk->data == orefllnk->data) {

					task->saved = g_list_delete_link(task->saved, orefllnk);

					GNUI_LIST_DELETE_AND_MOVE_TO_NEXT(
						&task->current,
						&crefllnk
					);

					goto process_current;

				}

				orefllnk = orefllnk->next;
				goto process_saved;

			}

			newlen++;
			crefllnk = crefllnk->next;
			goto process_current;

		}

		#undef orefllnk
		#undef crefllnk

		if (newlen > 0) {

			#define idx _size_2_
			#define crefllnk _list_placeholder_2_

			additions = g_new(const gchar *, newlen + 1);

			for (
				idx = 0, crefllnk = task->current;
					crefllnk && idx < newlen;
				additions[idx++] = ((EmblemReference *) crefllnk->data)->name,
				crefllnk = crefllnk->next
			);

			additions[idx] = NULL;

			#undef crefllnk
			#undef idx

		} else {

			additions = NULL;

		}

		#undef newlen
		#define oldlen _size_1_

		oldlen = g_list_length(task->saved);

		if (oldlen > 0) {

			#define idx _size_2_
			#define orefllnk _list_placeholder_2_

			removals = g_new(const gchar *, oldlen + 1);

			for (
				idx = 0, orefllnk = task->saved;
					orefllnk && idx < oldlen;
				removals[idx++] = ((EmblemReference *) orefllnk->data)->name,
				orefllnk = orefllnk->next
			);

			removals[idx] = NULL;

			#undef orefllnk
			#undef idx

		} else {

			removals = NULL;

		}

		#undef oldlen

		if (
			additions ||
			removals ||
			(flags & GNUI_EMBLEM_PICKER_SAVE_FLAG_SAVE_UNMODIFIED)
		) {

			if (
				g_file_set_attribute(
					task->location,
					G_FILE_ATTRIBUTE_METADATA_EMBLEMS,
					attribute_type,
					emblems_for_file,
					G_FILE_QUERY_INFO_NONE,
					cancellable,
					&saverr
				)
			) {

				result = GNUI_EMBLEM_PICKER_SUCCESS;
				fmatch = GNUI_EMBLEM_PICKER_SAVE_FLAG_CALLBACK_ON_SUCCESS;

			} else {

				result = GNUI_EMBLEM_PICKER_ERROR;
				fmatch = GNUI_EMBLEM_PICKER_SAVE_FLAG_CALLBACK_ON_ERROR;

			}

		} else {

			result = GNUI_EMBLEM_PICKER_NOACTION;
			fmatch = GNUI_EMBLEM_PICKER_SAVE_FLAG_CALLBACK_ON_NOACTION;

		}

		g_free(emblems_for_file);

		if (
			(
				for_each_file &&
				(flags & fmatch) &&
				!for_each_file(
					task->location,
					additions,
					removals,
					result,
					saverr,
					for_each_data
				)
			) || (
				result == GNUI_EMBLEM_PICKER_ERROR &&
				(flags & GNUI_EMBLEM_PICKER_SAVE_FLAG_ABORT_MODE)
			)
		) {

			while (taskllnk) {

				g_list_free(((FileEmblems *) taskllnk->data)->saved);
				g_list_free(((FileEmblems *) taskllnk->data)->current);
				g_free(taskllnk->data);
				taskllnk = taskllnk->next;

			}

			g_free(additions);
			g_free(removals);
			g_list_free(tasks);
			g_propagate_error(error, saverr);
			return false;

		}

		g_clear_error(&saverr);
		g_list_free(task->saved);
		g_list_free(task->current);
		g_free(additions);
		g_free(removals);

	}

	g_list_free_full(tasks, g_free);

	#undef taskllnk
	#define refllnk _cllink_1_

	if (flags & GNUI_EMBLEM_PICKER_SAVE_FLAG_CLEAN_INCONSISTENCY) {

		for (refllnk = priv->references; refllnk; refllnk = refllnk->next) {

			emref = refllnk->data;
			emref->saved_state = emref->current_state;
			g_clear_pointer(&emref->inconsistent_group, g_list_free);

		}

	} else {

		for (refllnk = priv->references; refllnk; refllnk = refllnk->next) {

			((EmblemReference *) refllnk->data)->saved_state =
				((EmblemReference *) refllnk->data)->current_state;

		}

	}

	if (self->modified) {

		self->modified = false;
		g_object_notify_by_pspec(G_OBJECT(self), props[PROPERTY_MODIFIED]);
		g_signal_emit(self, signals[SIGNAL_MODIFIED_CHANGED], 0, false);

	}

	if (~flags & GNUI_EMBLEM_PICKER_SAVE_FLAG_DONT_REFRESH) {

		for (refllnk = priv->references; refllnk; refllnk = refllnk->next) {

			gnui_emblem_picker_refresh_cell(refllnk->data);

		}

	}

	#undef refllnk

	return true;

}


gboolean gnui_emblem_picker_toggle_emblem (
	GnuiEmblemPicker * const self,
	const gchar * const emblem_name
) {

	g_return_val_if_fail(GNUI_IS_EMBLEM_PICKER(self), false);
	g_return_val_if_fail(emblem_name != NULL, false);

	const GnuiEmblemPickerPrivate * const priv =
		gnui_emblem_picker_get_instance_private(self);

	for (const GList * llnk = priv->references; llnk; llnk = llnk->next) {

		#define emref ((EmblemReference * ) llnk->data)

		if (!strcmp(emref->name, emblem_name)) {

			gnui_emblem_picker_toggle_emblem_reference(self, emref);
			return true;

		}

		#undef emref

	}

	return false;

}


gboolean gnui_emblem_picker_set_emblem_state (
	GnuiEmblemPicker * const self,
	const gchar * const emblem_name,
	const GnuiEmblemState state,
	const gboolean add_if_missing
) {

	g_return_val_if_fail(GNUI_IS_EMBLEM_PICKER(self), false);
	g_return_val_if_fail(emblem_name != NULL, false);

	GnuiEmblemPickerPrivate * const priv =
		gnui_emblem_picker_get_instance_private(self);

	EmblemReference * emref;

	for (const GList * llnk = priv->references; llnk; llnk = llnk->next) {

		emref = llnk->data;

		if (!strcmp(emref->name, emblem_name)) {

			if (
				state == GNUI_EMBLEM_STATE_INCONSISTENT &&
				!emref->inconsistent_group
			) {

				return false;

			}

			emref->current_state = state;
			gnui_emblem_picker_refresh_cell(emref);
			goto change_modif_and_exit;

		}

	}

	if (!add_if_missing) {

		return false;

	}

	if (self->forbidden_emblems) {

		const gchar * const * nameptr =
			(const gchar * const * ) self->forbidden_emblems - 1;

		while (*++nameptr) {

			if (!strcmp(emblem_name, *nameptr)) {

				return false;

			}

		}

	}

	emref = g_new(EmblemReference, 1);
	emref->name = g_strdup(emblem_name);
	emref->inconsistent_group = NULL;
	emref->saved_state = GNUI_EMBLEM_STATE_NORMAL;
	emref->current_state = state;
	emref->unsupported = true;
	priv->references = g_list_prepend(priv->references, emref);
	gnui_emblem_picker_draw_unsupported_emblem(self, priv, emref);
	gnui_emblem_picker_refresh_cell(emref);

	if (priv->is_single_page) {

		priv->is_single_page = false;
		gnui_emblem_picker_repage_view(self, priv);

	}


	/* \                                  /\
	\ */     change_modif_and_exit:      /* \
	 \/     ________________________     \ */


	if (!self->modified) {

		self->modified = true;
		g_object_notify_by_pspec(G_OBJECT(self), props[PROPERTY_MODIFIED]);
		g_signal_emit(self, signals[SIGNAL_MODIFIED_CHANGED], 0, true);

	}

	return true;

}


void gnui_emblem_picker_foreach (
	GnuiEmblemPicker * const self,
	const GnuiEmblemPickerForeachFunc foreach_func,
	const gpointer user_data
) {

	g_return_if_fail(GNUI_IS_EMBLEM_PICKER(self));
	g_return_if_fail(foreach_func != NULL);

	const GnuiEmblemPickerPrivate * const priv =
		gnui_emblem_picker_get_instance_private(self);

	const EmblemReference * emref;

	for (const GList * llnk = priv->references; llnk; llnk = llnk->next) {

		emref = llnk->data;

		if (
			!foreach_func(
				self,
				emref->name,
				emref->saved_state,
				emref->current_state,
				emref->inconsistent_group,
				user_data
			)
		) {

			break;

		}

	}

}


void gnui_emblem_picker_undo_all_selections (
	GnuiEmblemPicker * const self
) {

	g_return_if_fail(GNUI_IS_EMBLEM_PICKER(self));

	const GnuiEmblemPickerPrivate * const priv =
		gnui_emblem_picker_get_instance_private(self);

	EmblemReference * emref;

	for (const GList * llnk = priv->references; llnk; llnk = llnk->next) {

		emref = llnk->data;
		emref->current_state = emref->saved_state;
		gnui_emblem_picker_refresh_cell(emref);

	}

}


void gnui_emblem_picker_set_all_selections (
	GnuiEmblemPicker * const self,
	const gboolean selected
) {

	g_return_if_fail(GNUI_IS_EMBLEM_PICKER(self));

	const GnuiEmblemPickerPrivate * const priv =
		gnui_emblem_picker_get_instance_private(self);

	const GnuiEmblemState state =
		selected ?
			GNUI_EMBLEM_STATE_SELECTED
		:
			GNUI_EMBLEM_STATE_NORMAL;

	for (const GList * llnk = priv->references; llnk; llnk = llnk->next) {

		((EmblemReference *) llnk->data)->current_state = state;
		gnui_emblem_picker_refresh_cell(llnk->data);

	}

}


const GList * gnui_emblem_picker_peek_mapped_files (
	GnuiEmblemPicker * const self
) {

	g_return_val_if_fail(GNUI_IS_EMBLEM_PICKER(self), NULL);

	return (const GList *) self->mapped_files;

}


G_GNUC_WARN_UNUSED_RESULT GList * gnui_emblem_picker_get_mapped_files (
	GnuiEmblemPicker * const self
) {

	g_return_val_if_fail(GNUI_IS_EMBLEM_PICKER(self), NULL);

	GList * retval = g_list_copy(self->mapped_files);

	for (GList * llnk = retval; llnk; llnk = llnk->next) {

		g_object_ref(llnk->data);

	}

	return retval;

}


void gnui_emblem_picker_set_mapped_files (
	GnuiEmblemPicker * const self,
	const GList * const file_selection
) {

	g_return_if_fail(GNUI_IS_EMBLEM_PICKER(self));

	if (
		self->mapped_files ?
			gnui_lists_are_equal(file_selection, self->mapped_files)
		:
			!file_selection
	) {

		return;

	}

	self->mapped_files = g_list_copy((GList *) file_selection);

	for (GList * llnk = self->mapped_files; llnk; llnk = llnk->next) {

		g_object_ref(llnk->data);

	}

	gnui_emblem_picker_restart_session(
		self,
		gnui_emblem_picker_get_instance_private(self)
	);

	g_object_notify_by_pspec(G_OBJECT(self), props[PROPERTY_MAPPED_FILES]);

}


const gchar * const * gnui_emblem_picker_peek_forbidden_emblems (
	GnuiEmblemPicker * const self
) {

	g_return_val_if_fail(GNUI_IS_EMBLEM_PICKER(self), NULL);

	return (const gchar * const *) self->forbidden_emblems;

}


G_GNUC_WARN_UNUSED_RESULT gchar ** gnui_emblem_picker_get_forbidden_emblems (
	GnuiEmblemPicker * const self
) {

	g_return_val_if_fail(GNUI_IS_EMBLEM_PICKER(self), NULL);

	return g_strdupv((gchar **) self->forbidden_emblems);

}


void gnui_emblem_picker_set_forbidden_emblems (
	GnuiEmblemPicker * const self,
	const gchar * const * const forbidden_emblems
) {

	g_return_if_fail(GNUI_IS_EMBLEM_PICKER(self));

	if (
		(
			!forbidden_emblems && !self->forbidden_emblems
		) || (
			forbidden_emblems &&
			self->forbidden_emblems &&
			g_strv_equal(
				(const gchar * const *) self->forbidden_emblems,
				forbidden_emblems
			)
		)
	) {

		return;

	}

	g_strfreev(self->forbidden_emblems);
	self->forbidden_emblems = g_strdupv((gchar **) forbidden_emblems);

	gnui_emblem_picker_refresh_icons(
		self,
		gnui_emblem_picker_get_instance_private(self)
	);

	g_object_notify_by_pspec(
		G_OBJECT(self),
		props[PROPERTY_FORBIDDEN_EMBLEMS]
	);

}


gboolean gnui_emblem_picker_get_ensure_standard (
	GnuiEmblemPicker * const self
) {

	g_return_val_if_fail(GNUI_IS_EMBLEM_PICKER(self), false);

	return self->ensure_standard;

}


void gnui_emblem_picker_set_ensure_standard (
	GnuiEmblemPicker * const self,
	gboolean ensure_standard
) {

	g_return_if_fail(GNUI_IS_EMBLEM_PICKER(self));

	GnuiEmblemPickerPrivate * const priv =
		gnui_emblem_picker_get_instance_private(self);

	if (self->ensure_standard != ensure_standard) {

		self->ensure_standard = ensure_standard;

		if (ensure_standard) {

			priv->is_single_page &= gnui_emblem_picker_add_standard_emblems(
				self,
				priv
			);

			gnui_emblem_picker_repage_view(self, priv);

		} else {

			gnui_emblem_picker_refresh_icons(
				self,
				gnui_emblem_picker_get_instance_private(self)
			);

		}

		g_object_notify_by_pspec(
			G_OBJECT(self),
			props[PROPERTY_ENSURE_STANDARD]
		);

	}

}


gboolean gnui_emblem_picker_get_modified (
	GnuiEmblemPicker * const self
) {

	g_return_val_if_fail(GNUI_IS_EMBLEM_PICKER(self), false);

	return self->modified;

}


void gnui_emblem_picker_set_modified (
	GnuiEmblemPicker * const self,
	const gboolean modified
) {

	g_return_if_fail(GNUI_IS_EMBLEM_PICKER(self));

	if (self->modified != modified) {

		self->modified = modified;
		g_object_notify_by_pspec(G_OBJECT(self), props[PROPERTY_MODIFIED]);
		g_signal_emit(self, signals[SIGNAL_MODIFIED_CHANGED], 0, modified);

	}

}


gboolean gnui_emblem_picker_get_reveal_changes (
	GnuiEmblemPicker * const self
) {

	g_return_val_if_fail(GNUI_IS_EMBLEM_PICKER(self), false);

	return self->reveal_changes;

}


void gnui_emblem_picker_set_reveal_changes (
	GnuiEmblemPicker * const self,
	const gboolean reveal_changes
) {

	g_return_if_fail(GNUI_IS_EMBLEM_PICKER(self));

	if (self->reveal_changes != reveal_changes) {

		self->reveal_changes = reveal_changes;

		g_object_notify_by_pspec(
			G_OBJECT(self),
			props[PROPERTY_REVEAL_CHANGES]
		);

	}

}


G_GNUC_WARN_UNUSED_RESULT GtkWidget * gnui_emblem_picker_new (
	GList * const mapped_files,
	const gboolean ensure_standard,
	const gchar * const * const forbidden_emblems
) {

	return GTK_WIDGET(
		g_object_new(
			gnui_emblem_picker_get_type(),
			"mapped-files", mapped_files,
			"forbidden-emblems", forbidden_emblems,
			"ensure-standard", ensure_standard,
			NULL
		)
	);

}


/*  EOF  */

