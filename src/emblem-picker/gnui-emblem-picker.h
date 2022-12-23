/*  -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-  */

/*\
|*|
|*| gnui-emblem-picker.h
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



#ifndef _GNUI_EMBLEM_PICKER_H_
#define _GNUI_EMBLEM_PICKER_H_


#include <gtk/gtk.h>


G_BEGIN_DECLS


/**

    SECTION:gnui-emblem-picker
    @title: GnuiEmblemPicker
    @short_description: An emblem picker
    @section_id: gnui-emblem-picker
    @stability: Unstable
    @include: gnuisance/gnui-emblem-picker.h
    @image: gnui-emblem-picker-screenshot.png

    The `GnuiEmblemPicker` widget allows interactively to manipulate the emblem
    metadata of one or more files, providing support for inconsistent states.

**/


/**

    GNUI_TYPE_EMBLEM_PICKER:

    The `GType` of `GnuiEmblemPicker`

**/
#define GNUI_TYPE_EMBLEM_PICKER (gnui_emblem_picker_get_type())


/**

    GnuiEmblemPicker:

    The `GnuiEmblemPicker` widget

**/
G_DECLARE_FINAL_TYPE(
    GnuiEmblemPicker,
    gnui_emblem_picker,
    GNUI,
    EMBLEM_PICKER,
    GtkWidget
)


#ifndef __GTK_DOC_IGNORE__
#define GNUI_EMBLEM_STATES_WITH_PREFIX(PREFIX) \
    PREFIX##NORMAL, PREFIX##SELECTED, PREFIX##INCONSISTENT
#endif


/**

    GnuiEmblemState:
    GNUI_EMBLEM_STATE_NORMAL: Blablabla
    GNUI_EMBLEM_STATE_SELECTED: Blablabla
    GNUI_EMBLEM_STATE_INCONSISTENT: Blablabla

    Possible states of an emblem cell

**/
typedef enum GnuiEmblemState {
    GNUI_EMBLEM_STATES_WITH_PREFIX(GNUI_EMBLEM_STATE_)
} GnuiEmblemState;


/**

    GnuiEmblemPickerSaveFlags:
    @GNUI_EMBLEM_PICKER_SAVE_FLAG_NONE:                 No flags
    @GNUI_EMBLEM_PICKER_SAVE_FLAG_SAVE_UNMODIFIED:      Re-write the files'
                                                        metadata even if there
                                                        are no changes
    @GNUI_EMBLEM_PICKER_SAVE_FLAG_ABORT_MODE:           Stop the save
                                                        operations as soon as a
                                                        file cannot be saved
                                                        (without it the save
                                                        operations will
                                                        continue until the end)
    @GNUI_EMBLEM_PICKER_SAVE_FLAG_DONT_REFRESH:         Do not refresh the
                                                        state indicators
                                                        (useful if this is a
                                                        last save before
                                                        destroying the emblem
                                                        picker widget)
    @GNUI_EMBLEM_PICKER_SAVE_FLAG_CLEAN_INCONSISTENCY:  Erase the information
                                                        about the emblem's
                                                        original inconsistency
                                                        if this is not saved as
                                                        inconsistent
    @GNUI_EMBLEM_PICKER_SAVE_FLAG_CALLBACK_ON_ERROR:    Invoke the user-given
                                                        callback on errors
    @GNUI_EMBLEM_PICKER_SAVE_FLAG_CALLBACK_ON_SUCCESS:  Invoke the user-given
                                                        callback when the file
                                                        was successfully
                                                        updated with the new
                                                        emblems
    @GNUI_EMBLEM_PICKER_SAVE_FLAG_CALLBACK_ON_NOACTION: Invoke the user-given
                                                        callback when no
                                                        actions were performed
                                                        on the file
    @GNUI_EMBLEM_PICKER_SAVE_FLAGGROUP_ALL_CALLBACKS:   Always invoke the
                                                        user-given callback, no
                                                        matter what the save
                                                        result is
    @GNUI_EMBLEM_PICKER_SAVE_FLAGGROUP_ALL_FLAGS_SET:   Convenience identifier
                                                        in which all flags are
                                                        set

    Possible options for `gnui_emblem_picker_save()`

**/
typedef enum {
    GNUI_EMBLEM_PICKER_SAVE_FLAG_NONE = 0,
    GNUI_EMBLEM_PICKER_SAVE_FLAG_SAVE_UNMODIFIED = 1,
    GNUI_EMBLEM_PICKER_SAVE_FLAG_ABORT_MODE = 2,
    GNUI_EMBLEM_PICKER_SAVE_FLAG_DONT_REFRESH = 4,
    GNUI_EMBLEM_PICKER_SAVE_FLAG_CLEAN_INCONSISTENCY = 8,
    GNUI_EMBLEM_PICKER_SAVE_FLAG_CALLBACK_ON_ERROR = 16,
    GNUI_EMBLEM_PICKER_SAVE_FLAG_CALLBACK_ON_SUCCESS = 32,
    GNUI_EMBLEM_PICKER_SAVE_FLAG_CALLBACK_ON_NOACTION = 64,
    GNUI_EMBLEM_PICKER_SAVE_FLAGGROUP_ALL_CALLBACKS =
        GNUI_EMBLEM_PICKER_SAVE_FLAG_CALLBACK_ON_ERROR |
        GNUI_EMBLEM_PICKER_SAVE_FLAG_CALLBACK_ON_SUCCESS |
        GNUI_EMBLEM_PICKER_SAVE_FLAG_CALLBACK_ON_NOACTION,
    GNUI_EMBLEM_PICKER_SAVE_FLAGGROUP_ALL_FLAGS_SET = 127
} GnuiEmblemPickerSaveFlags;


/**

    GnuiEmblemPickerSaveResult:
    @GNUI_EMBLEM_PICKER_ERROR:      The file's emblem metadata could not be
                                    saved
    @GNUI_EMBLEM_PICKER_NOACTION:   No actions were necessary for the file's
                                    metadata
    @GNUI_EMBLEM_PICKER_SUCCESS:    The file's emblem metadata were
                                    successfully saved

    Possible results of `gnui_emblem_picker_save()` on each file

**/
typedef enum GnuiEmblemPickerSaveResult {
    GNUI_EMBLEM_PICKER_ERROR = -1,
    GNUI_EMBLEM_PICKER_NOACTION = 0,
    GNUI_EMBLEM_PICKER_SUCCESS = 1,
} GnuiEmblemPickerSaveResult;



/*  Callback types  */


/**

    GnuiEmblemPickerForeachFunc:
    @self:                  (auto) (not nullable): The emblem picker
    @emblem_name:           (auto) (not nullable): The emblem's name
    @saved_state:           (auto): The last saved state of the emblem
    @current_state:         (auto): The current state of the emblem
    @inconsistent_group:    (auto) (transfer none) (nullable): The inconsistent
                            group of `GFile` handles that can have the emblem
                            assigned independently of the other files
    @user_data:             (auto) (nullable) (closure): The custom data
                            provided by the user

    A callback function type for `gnui_emblem_picker_foreach()`

    Returns:    `true` if the for-each loop must continue, `false` otherwise

**/
typedef gboolean (* GnuiEmblemPickerForeachFunc) (
    GnuiEmblemPicker * self,
    const gchar * emblem_name,
    GnuiEmblemState saved_state,
    GnuiEmblemState current_state,
    const GList * inconsistent_group,
    gpointer user_data
);


/**

    GnuiEmblemPickerForeachSavedFileFunc:
    @location:          (auto) (not nullable): The file for which the function
                        is being called
    @added_emblems:     (auto) (transfer none) (nullable): The emblems added to
                        the file, or `NULL`
    @removed_emblems:   (auto) (transfer none) (nullable): The emblems removed
                        from the file, or `NULL`
    @result:            (auto): The result of the save operation
    @error:             (auto) (transfer none) (nullable): The error
    @user_data:         (auto) (nullable) (closure): The custom data provided
                        by the user

    A callback function type for each saved file

    Returns:    `true` if the saving operations must carry on, `false`
                otherwise

**/
typedef gboolean (* GnuiEmblemPickerForeachSavedFileFunc) (
    GFile * location,
    const gchar * const * added_emblems,
    const gchar * const * removed_emblems,
    GnuiEmblemPickerSaveResult result,
    const GError * error,
    gpointer user_data
);



/*  Functions  */


/**

    gnui_emblem_picker_new:
    @file_selection:    (transfer none) (nullable): A `GList` containing
                        `GFile` handles to map for their emblems
    @enforce_xdg:       If `true`, always show XDG standard emblems in the
                        emblem picker, even if these are not supported by the
                        current theme
    @forbidden_emblems: (transfer none) (nullable): The emblems to exclude

    Create a new emblem picker widget

    For the list of XDG standard emblems, please refer to
    [Icon Naming Specification, Table 6. Standard Emblem Icons](https://specifications.freedesktop.org/icon-naming-spec/icon-naming-spec-0.4.html#idm45098400102912).

    Returns:    The newly created emblem picker widget

**/
extern GtkWidget * gnui_emblem_picker_new (
    GList * const file_selection,
    const gboolean enforce_xdg,
    const gchar * const * const forbidden_emblems
) G_GNUC_WARN_UNUSED_RESULT;


/**

    gnui_emblem_picker_save:
    @self:          (not nullable): The emblem picker
    @for_each_file: (nullable): A function to call for each saved file
                    according to the conditions expressed by the @flag
                    parameter
    @flags:         Save options flags
    @cancellable:   (nullable): A `GCancellable`
    @error:         (nullable): A `GError` or `NULL`; ignored unless
                    `GNUI_EMBLEM_PICKER_SAVE_FLAG_ABORT_MODE` is present in
                    @flags
    @for_each_data: (nullable): The custom data to pass to @for_each_file

    Save the emblems currently selected into the files' metadata

    Returns:    `true` if all the emblems were successfully saved, `false`
                otherwise

**/
extern gboolean gnui_emblem_picker_save (
    GnuiEmblemPicker * const self,
    const GnuiEmblemPickerForeachSavedFileFunc for_each_file,
    const GnuiEmblemPickerSaveFlags flags,
    GCancellable * const cancellable,
    GError ** const error,
    const gpointer for_each_data
);


/**

    gnui_emblem_picker_refresh_states:
    @self:      (not nullable): The emblem picker

    Refresh the emblem picker's indicator glyphs

    This function is useless unless you have just invoked
    `gnui_emblem_picker_save()` with
    `GNUI_EMBLEM_PICKER_SAVE_FLAG_DONT_REFRESH`.

**/
extern void gnui_emblem_picker_refresh_states (
    GnuiEmblemPicker * const self
);


/**

    gnui_emblem_picker_toggle_emblem:
    @self:          (not nullable): The emblem picker
    @emblem_name:   (transfer none) (not nullable): The name of the emblem to
                    toggle

    Toggle/untoggle an emblem picker's emblem

    This function behaves exactly like toggling/untoggling an emblem via user
    input.

**/
extern gboolean gnui_emblem_picker_toggle_emblem (
    GnuiEmblemPicker * const self,
    const gchar * const emblem_name
);


/**

    gnui_emblem_picker_set_emblem_state:
    @self:              (not nullable): The emblem picker
    @emblem_name:       (transfer none) (not nullable): The name of the emblem
                        to set
    @state:             The state to assign to the emblem
    @add_if_missing:    If `true`, create the emblem when the emblem is not
                        found

    Toggle/untoggle an emblem picker's emblem

    When @add_if_missing is set to false this function behaves exactly like
    toggling/untoggling an emblem via user input.

**/
extern gboolean gnui_emblem_picker_set_emblem_state (
    GnuiEmblemPicker * const self,
    const gchar * const emblem_name,
    const GnuiEmblemState state,
    const gboolean add_if_missing
);


/**

    gnui_emblem_picker_peek_mapped_files: (get-property mapped-files)
    @self:      (not nullable): The emblem picker

    Get the `const` list of the files mapped by an emblem picker

    Returns:    (transfer none): A `GList` containing `GFile` handles (do not
                free or modify it)

**/
extern const GList * gnui_emblem_picker_peek_mapped_files (
    GnuiEmblemPicker * const self
);


/**

    gnui_emblem_picker_get_mapped_files:
    @self:      (not nullable): The emblem picker

    Get the list of the files mapped by an emblem picker

    Returns:    (transfer full): A newly allocated `GList` containing `GFile`
                handles (free it with `g_list_free_full(mapped_files,
                g_object_unref)`)

**/
extern GList * gnui_emblem_picker_get_mapped_files (
    GnuiEmblemPicker * const self
) G_GNUC_WARN_UNUSED_RESULT;


/**

    gnui_emblem_picker_set_mapped_files: (set-property mapped-files)
    @self:              (not nullable): The emblem picker
    @file_selection:    (transfer none) (nullable): A `GList` containing
                        `GFile` handles to map for their emblems

    Set the list of the files mapped by an emblem picker

    Changing the emblem picker's files is similar to starting a new session
    (i.e. creating a new emblem picker). The current emblem selection will be
    lost.

**/
extern void gnui_emblem_picker_set_mapped_files (
    GnuiEmblemPicker * const self,
    const GList * const file_selection
);


/**

    gnui_emblem_picker_peek_forbidden_emblems: (get-property forbidden-emblems)
    @self:      (not nullable): The emblem picker

    Get the `const` array of strings of the emblems that must not be displayed
    in the emblem picker

    Returns:    (transfer none): An array of strings containing the names of
                the emblems that must not be displayed in the emblem picker (do
                not free or modify it)

**/
extern const gchar * const * gnui_emblem_picker_peek_forbidden_emblems (
    GnuiEmblemPicker * const self
);


/**

    gnui_emblem_picker_get_forbidden_emblems:
    @self:      (not nullable): The emblem picker

    Get the emblems that must not be displayed in the emblem picker

    Returns:    (transfer full): A newly allocated array of strings containing
                the names of the emblems that must not be displayed in the
                emblem picker (free the returned array with `g_strfreev()`)

**/
extern gchar ** gnui_emblem_picker_get_forbidden_emblems (
    GnuiEmblemPicker * const self
) G_GNUC_WARN_UNUSED_RESULT;


/**

    gnui_emblem_picker_set_forbidden_emblems: (set-property forbidden-emblems)
    @self:              (not nullable): The emblem picker
    @forbidden_emblems: (transfer none): An array of strings containing the
                        names of the emblems that must not be displayed in the
                        emblem picker

    Set the emblems that must not be displayed in the emblem picker

**/
extern void gnui_emblem_picker_set_forbidden_emblems (
    GnuiEmblemPicker * const self,
    const gchar * const * const forbidden_emblems
);


/**

    gnui_emblem_picker_get_ensure_standard: (get-property ensure-standard)
    @self:      (not nullable): The emblem picker

    Get whether XDG standard emblems must always be displayed, even if the
    current icon theme does not support them

    For the list of XDG standard emblems, please refer to
    [Icon Naming Specification, Table 6. Standard Emblem Icons](https://specifications.freedesktop.org/icon-naming-spec/icon-naming-spec-0.4.html#idm45098400102912).

    Returns:    `true` if XDG standard emblems must always be displayed,
                `false` otherwise

**/
extern gboolean gnui_emblem_picker_get_ensure_standard (
    GnuiEmblemPicker * const self
);


/**

    gnui_emblem_picker_set_ensure_standard: (set-property ensure-standard)
    @self:              (not nullable): The emblem picker
    @ensure_standard:   `true` if XDG standard emblems must always be
                        displayed, even if the current icon theme does not
                        support them; `false` otherwise

    Set whether XDG standard emblems must always be displayed, even if the
    current icon theme does not support them

    For the list of XDG standard emblems, please refer to
    [Icon Naming Specification, Table 6. Standard Emblem Icons](https://specifications.freedesktop.org/icon-naming-spec/icon-naming-spec-0.4.html#idm45098400102912).

**/
extern void gnui_emblem_picker_set_ensure_standard (
    GnuiEmblemPicker * const self,
    gboolean ensure_standard
);


/**

    gnui_emblem_picker_get_modified: (get-property modified)
    @self:      (not nullable): The emblem picker

    Get whether the emblem selection has been modified by the user

    Returns:    Whether the emblem selection has been modified by the user

**/
extern gboolean gnui_emblem_picker_get_modified (
    GnuiEmblemPicker * const self
);


/**

    gnui_emblem_picker_set_modified: (set-property modified)
    @self:      (not nullable): The emblem picker
    @modified:  `true` if the emblem picker's state must be set to "modified",
                `false` otherwise

    Set whether the emblem selection has been modified by the user

**/
extern void gnui_emblem_picker_set_modified (
    GnuiEmblemPicker * const self,
    gboolean modified
);


/**

    gnui_emblem_picker_get_reveal_changes: (get-property reveal-changes)
    @self:      (not nullable): The emblem picker

    Get whether an indicator glyph must be displayed for each emblem whose
    selection state has changed since last save

    Returns:    `true` if an indicator glyph must be displayed for each emblem
                whose selection state has changed, `false` otherwise

**/
extern gboolean gnui_emblem_picker_get_reveal_changes (
    GnuiEmblemPicker * const self
);


/**

    gnui_emblem_picker_set_reveal_changes: (set-property reveal-changes)
    @self:              (not nullable): The emblem picker
    @reveal_changes:    `true` if an indicator glyph must be displayed for each
                        emblem whose selection state has changed, `false`
                        otherwise

    Set whether an indicator glyph must be displayed for each emblem whose
    selection state has changed since last save

**/
extern void gnui_emblem_picker_set_reveal_changes (
    GnuiEmblemPicker * const self,
    const gboolean reveal_changes
);


/**

    gnui_emblem_picker_foreach:
    @self:          (not nullable): The emblem picker
    @foreach_func:  (not nullable): A function called for each emblem displayed
                    by the emblem picker
    @user_data:     (nullable) (closure): The closure to pass to @foreach_func

    Call a custom function for each emblem displayed by the emblem picker

**/
extern void gnui_emblem_picker_foreach (
    GnuiEmblemPicker * const self,
    const GnuiEmblemPickerForeachFunc foreach_func,
    const gpointer user_data
);


/**

    gnui_emblem_picker_undo_all_selections:
    @self:      (not nullable): The emblem picker

    Bring an emblem picker selection state back to the last saved state

**/
extern void gnui_emblem_picker_undo_all_selections (
    GnuiEmblemPicker * const self
);


/**

    gnui_emblem_picker_set_all_selections:
    @self:      (not nullable): The emblem picker
    @selected:  `true` if the selection state must be set to `selected`,
                `false` for the opposite

    Set the selection state of all the emblems of an emblem picker

**/
extern void gnui_emblem_picker_set_all_selections (
    GnuiEmblemPicker * const self,
    const gboolean selected
);



/*  Signal handler types  */


/**

    GnuiEmblemPickerSignalHandlerEmblemSelected:
    @self:                          (auto) (not nullable): The emblem picker
    @emblem_name:                   (auto) (transfer none) (not nullable): The
                                    emblem name
    @saved_state:                   (auto) The last saved state of the emblem
    @current_state:                 (auto) The current state of the emblem
    @inconsistent_group:            (auto) (transfer none) (nullable): The
                                    inconsistent group of `GFile` handles that
                                    can have the emblem assigned independently
                                    from the other files, or `NULL`
    @user_data:                     (auto) (nullable) (closure): The custom
                                    data passed to the signal

    A handler function type for the #GnuiEmblemPicker::emblem-selected signal

**/
typedef void (* GnuiEmblemPickerSignalHandlerEmblemSelected) (
    GnuiEmblemPicker * self,
    const gchar * emblem_name,
    GnuiEmblemState saved_state,
    GnuiEmblemState current_state,
    const GList * inconsistent_group,
    gpointer user_data
);


/**

    GnuiEmblemPickerSignalHandlerModifiedChanged:
    @self:      (auto) (not nullable): The emblem picker
    @modified:  (auto): The current state of the `"modified"` property
    @user_data: (auto) (nullable) (closure): The custom data passed to the
                signal

    A handler function type for the #GnuiEmblemPicker::modified-changed signal

**/
typedef void (* GnuiEmblemPickerSignalHandlerModifiedChanged) (
    GnuiEmblemPicker * self,
    gboolean modified,
    gpointer user_data
);


G_END_DECLS


#endif


/*  EOF  */
