/*  -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-  */

/*\
|*|
|*| gnui-definitions.h
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



#ifndef _GNUI_DEFINITIONS_H_
#define _GNUI_DEFINITIONS_H_


#include <gtk/gtk.h>


G_BEGIN_DECLS


/**

    SECTION:gnui-definitions
    @title: Miscellaneous macros and inline functions
    @section_id: gnui-definitions
    @stability: Unstable
    @include: gnuisance/gnui-definitions.h

    These macros provide more specialized features which are not needed so
    often by application programmers.

**/



/*\
|*|
|*| INTERNAL PREPROCESSOR SUGAR
|*|
\*/



#define _GNUI_PPX_ARG2_(ARG1, ARG2, ...) ARG2
#define _GNUI_PPX_ARG2_OR_ZERO_(...) _GNUI_PPX_ARG2_(__VA_ARGS__, 0,)
#define _GNUI_PPX_NOT_0_ 1
#define _GNUI_PPX_NOT_1_ 0
#define _GNUI_PPX_NEGA_(BOOL) GNUI_PP_LITERAL_PASTE3(_GNUI_PPX_NOT_, BOOL, _)
#define _GNUI_PPX_COMMA_ONE_ , 1
#define _GNUI_PPX_ARG_COMMA_ONE_COMMA_(ARG) ARG, 1,
#define _GNUI_PPX_UINT_NOT_0_ _GNUI_PPX_ARG_COMMA_ONE_COMMA_(~)
#define _GNUI_PPX_COND_0_(IF_TRUE, IF_FALSE, ...) IF_FALSE
#define _GNUI_PPX_COND_1_(IF_TRUE, ...) IF_TRUE



/*\
|*|
|*| GENERAL PREPROCESSOR UTILITIES
|*|
\*/


/**

    GNUI_PP_CALL:
    @MACRO:     The function-like macro to call
    @...:       The arguments to pass to @MACRO

    Call a function-like macro

    Returns:    The return value of @MACRO

**/
#define GNUI_PP_CALL(MACRO, ...) MACRO(__VA_ARGS__)


/**

    GNUI_PP_EAT:
    @...:       The preprocessor code to eat

    Eat the given preprocessor code

**/
#define GNUI_PP_EAT(...)


/**

    GNUI_PP_EXPAND:
    @...:       The preprocessor code to expand

    Expand the given preprocessor code

    Returns:    The expanded preprocessor code

**/
#define GNUI_PP_EXPAND(...) __VA_ARGS__


/**

    GNUI_PP_PASTE2:
    @MACRO1:    The first macro name
    @MACRO2:    The second macro name

    Paste together the expansion of two macros

    Returns:    The expansions of the two macros @MACRO1 and @MACRO2 pasted
                together

**/
#define GNUI_PP_PASTE2(MACRO, ...) GNUI_PP_LITERAL_PASTE2(MACRO, __VA_ARGS__)


/**

    GNUI_PP_PASTE3:
    @MACRO1:    The first macro name
    @MACRO2:    The second macro name
    @MACRO3:    The third macro name

    Paste together the expansion of three macros

    Returns:    The expansions of the three macros @MACRO1, @MACRO2 and @MACRO3
                pasted together

**/
#define GNUI_PP_PASTE3(MACRO1, MACRO2, ...) \
    GNUI_PP_LITERAL_PASTE3(MACRO1, MACRO2, __VA_ARGS__)


/**

    GNUI_PP_STRINGIFY:
    @MACRO:     The macro to stringify after being expanded

    Stringify the expansion of a macro

    Returns:    The @MACRO argument stringified after being expanded

**/
#define GNUI_PP_STRINGIFY(MACRO) GNUI_PP_LITERAL_STRINGIFY(MACRO)


/**

    GNUI_PP_BOOL:
    @COND:      The condition to check

    Transform a condition into a literal boolean

    Returns:    `1` if @COND is non-zero or empty, `0` otherwise

**/
#define GNUI_PP_BOOL(COND) _GNUI_PPX_NEGA_(GNUI_PP_BOOL_NOT(COND))


/**

    GNUI_PP_BOOL_NOT:
    @COND:      The condition to check

    Transform a condition into a literal boolean and negate the result

    Returns:    `0` if @COND is non-zero or empty, `1` otherwise

**/
#define GNUI_PP_BOOL_NOT(COND) \
    _GNUI_PPX_ARG2_OR_ZERO_(GNUI_PP_LITERAL_PASTE3(_GNUI_PPX_UINT_NOT_, COND, _))


/**

    GNUI_PP_IF_NONZERO:
    @COND:      The condition to check
    @IF_TRUE:   The preprocessor code to expand if @COND is non-zero
    @IF_FALSE:  (omittable): The preprocessor code to expand if @COND is zero

    Inline preprocessor if-else

    Returns:    @IF_TRUE if @COND is non-zero, @IF_FALSE otherwise

**/
#define GNUI_PP_IF_NONZERO(COND, ...) \
    GNUI_PP_IF_ONE_OR_ZERO(GNUI_PP_BOOL(COND), __VA_ARGS__)


/**

    GNUI_PP_IF_ONE_OR_ZERO:
    @COND:      The condition to check
    @IF_TRUE:   The preprocessor code to expand if @COND is `1`
    @IF_FALSE:  (omittable): The preprocessor code to expand if @COND is `0`

    Check if a condition is exactly one or zero

    If @COND is neither `0` nor `1` the behavior is undefined.

    Returns:    @IF_TRUE if @COND is `1`, @IF_FALSE if @COND is `0`

**/
#define GNUI_PP_IF_ONE_OR_ZERO(COND, ...) \
    GNUI_PP_LITERAL_PASTE3(_GNUI_PPX_COND_, COND, _)(__VA_ARGS__, , )


/**

    GNUI_PP_IS_BLANK:
    @MACRO:     The macro to check

    Check if a macro is defined *and* blank

    Returns:    `1` if @MACRO is defined *and* blank, `0` otherwise

**/
#define GNUI_PP_IS_BLANK(MACRO) \
    GNUI_PP_CALL( \
        _GNUI_PPX_ARG2_, \
        GNUI_PP_LITERAL_PASTE2(_GNUI_PPX_COMMA_ONE_, MACRO), \
        0, \
    )


/**

    GNUI_PP_LITERAL_PASTE2:
    @MACRO1:    The first macro name
    @MACRO2:    The second macro name

    Paste two macro names without expanding them

    Returns:    The two macro names @MACRO1 and @MACRO2 pasted together,
                without performing macro expansion

**/
#define GNUI_PP_LITERAL_PASTE2(MACRO, ...) MACRO##__VA_ARGS__


/**

    GNUI_PP_LITERAL_PASTE3:
    @MACRO1:    The first macro name
    @MACRO2:    The second macro name
    @MACRO3:    The third macro name

    Paste three macro names without expanding them

    Returns:    The three macro names @MACRO1, @MACRO2 and @MACRO3 pasted
                together, without performing macro expansion

**/
#define GNUI_PP_LITERAL_PASTE3(MACRO1, MACRO2, ...) MACRO1##MACRO2##__VA_ARGS__


/**

    GNUI_PP_LITERAL_STRINGIFY:
    @MACRO:     The macro name to stringify

    Stringify a macro name without expanding it

    Returns:    The @MACRO argument stringified, without expansions

**/
#define GNUI_PP_LITERAL_STRINGIFY(MACRO) #MACRO


/**

    GNUI_PP_WHEN_NONZERO:
    @MACRO:     The macro to expand

    Expand a macro only if non-zero

    Returns:    The expansion @MACRO only if it is non-zero, nothing otherwise

**/
#define GNUI_PP_WHEN_NONZERO(MACRO) \
    GNUI_PP_IF_NONZERO(MACRO, GNUI_PP_EXPAND, GNUI_PP_EAT)(MACRO)



/*\
|*|
|*| MACHINERY SUGAR
|*|
\*/


/**

    GNUI_GNUC_CONSTRUCTOR:

    Mark a function as auto-init

    Returns:    The attribute to include

**/
#define GNUI_GNUC_CONSTRUCTOR __attribute__((constructor))


/**

    GNUI_CONTAINER_OF:
    @PTR:       The pointed address
    @TYPE:      The type of `struct` expected
    @MEMBER:    The name of the member the pointed address refers to

    Get the `struct` that contains the pointed address as one of its members

    Returns:    The container `struct`

**/
#define GNUI_CONTAINER_OF(PTR, TYPE, MEMBER) \
    ((TYPE *) ((char *) (PTR) - offsetof(TYPE, MEMBER)))



/*\
|*|
|*| GENERIC SUGAR
|*|
\*/


/**

    GNUI_SET_POINTED_VALUE_IF_GIVEN__2_1:
    @PTR:       (out) (optional): The pointer
    @VAL:       The value

    Set a pointed value only if the pointer is not `NULL`

**/
#define GNUI_SET_POINTED_VALUE_IF_GIVEN__2_1(PTR, VAL) \
    if (PTR) *(PTR) = (VAL)


/**

    GNUI_SET_IF_GREATER__2_2:
    @VAR:       The variable
    @VAL:       The value

    Assign a value to a variable only if the value to assign is greater than
    the current value of the variable

**/
#define GNUI_SET_IF_GREATER__2_2(VAR, VAL) \
    if ((VAL) > (VAR)) (VAR) = (VAL)


/**

    GNUI_NORMALIZE_INT_UINT_CAST__2:
    @UINT:      The unsigned variable to sanitize

    Sanitize a cast between a signed and an unsigned integer

**/
#define GNUI_NORMALIZE_INT_UINT_CAST__2(UINT) \
    if (*((gint *) &(UINT)) < 0) (UINT) = 0


/**

    GNUI_ROUND_UINT_DIVISION__1_2:
    @NUM:       The numerator
    @DEN:       The dividend

    Round a division between two unsigned integers without float computations

    Returns:    The rounded result

**/
#define GNUI_ROUND_UINT_DIVISION__1_2(NUM, DEN) \
    ((NUM + DEN / 2) / DEN)


/**

    GNUI_ROUND_INT_DIVISION__2_3:
    @NUM:       The numerator
    @DEN:       The dividend

    Round a division between two signed integers

    Returns:    The rounded result

**/
#define GNUI_ROUND_INT_DIVISION__2_3(NUM, DEN) \
    ((NUM < 0) ^ (DEN < 0) ? (NUM - DEN / 2) / DEN : (NUM + DEN / 2) / DEN)


/**

    GNUI_PERPENDICULAR_ORIENTATION:
    @ORIENTATION    The original `GtkOrientation`

    Find the perpendicular `GtkOrientation`

    Returns:    The perpendicular `GtkOrientation`

**/
#define GNUI_PERPENDICULAR_ORIENTATION(ORIENTATION) ( \
    (ORIENTATION) == GTK_ORIENTATION_VERTICAL ? \
        GTK_ORIENTATION_HORIZONTAL \
    : \
        GTK_ORIENTATION_VERTICAL \
)


/**

    gnui_list_prepend_llink:
    @list:              (nullable): The doubly-linked list
    @detached_llink:    (not nullable): A detached list link

    Prepend a detached list link to a list

    Returns:    The new beginning of the list

**/
static inline GList * gnui_list_prepend_llink (
    GList * const list,
    GList * const detached_llink
) {
    detached_llink->prev = NULL;
    detached_llink->next = list;
    if (list) list->prev = detached_llink;
    return detached_llink;
}


/**

    gnui_list_u_detach_and_move_to_next:
    @list:      (out) (not nullable): A pointer to the doubly-linked list
    @llink:     (out) (not nullable): A pointer that before the call must point
                to the member to remove and which after the call will be set to
                the next member in the list

    Like `gnui_list_detach_and_move_to_next()`, but the `.prev` and `.next`
    members of the removed element are not set to `NULL` (please use it only if
    you know what you are doing)

    Please refer to `gnui_list_detach_and_move_to_next()` for an example.

    Returns:    The removed element

**/
static inline GList * gnui_list_u_detach_and_move_to_next (
    GList ** const list,
    GList ** const llink
) {
    GList * del = *llink, * nxt = del->next, * prv = del->prev;
    if (nxt) nxt->prev = prv;
    if (prv) prv->next = nxt;
    if (*list == del) *list = nxt;
    *llink = nxt;
    return del;
}


/**

    GNUI_LIST_DELETE_AND_MOVE_TO_NEXT:
    @LIST:      (out) (not nullable): A pointer to the doubly-linked list
    @LLINK:     (out) (not nullable): A pointer that before the call must point
                to the member to remove and which after the call will be set to
                the next member in the list

    Removes a pointed member from a `GList`, set the pointer to the next member
    in the list (or to `NULL` if the list is over) and free the removed
    element.

    This function is only sugar for loops (useful for deleting elements from a
    list while the loop must keep going).

    Writing

    |[<!-- language="C" -->
    GNUI_LIST_DELETE_AND_MOVE_TO_NEXT(&list, &current);
    ]|

    is equivalent to writing

    |[<!-- language="C" -->
    removed = current;
    current = current->next;
    list = g_list_remove_link(list, removed);
    g_list_free_1(removed);
    ]|

**/
#define GNUI_LIST_DELETE_AND_MOVE_TO_NEXT(LIST, LLINK) \
    g_list_free_1(gnui_list_u_detach_and_move_to_next((LIST), (LLINK)))


/**

    gnui_list_detach_and_move_to_next:
    @list:      (out) (not nullable): A pointer to the doubly-linked list
    @llink:     (out) (not nullable): A pointer that before the call must point
                to the member to remove and which after the call will be set to
                the next member in the list

    Removes a pointed member from a `GList`, set the pointer to the next member
    in the list (or to `NULL` if the list is over) and return the removed
    element.

    This function is only sugar for loops (useful for removing elements from a
    list while the loop must keep going).

    Writing

    |[<!-- language="C" -->
    do_something(gnui_list_detach_and_move_to_next(&list, &current));
    ]|

    is equivalent to writing

    |[<!-- language="C" -->
    removed = current;
    current = current->next;
    list = g_list_remove_link(list, removed);
    do_something(removed);
    ]|

    Returns:    The removed element

**/
static inline GList * gnui_list_detach_and_move_to_next (
    GList ** const list,
    GList ** const llink
) {
    GList * del = *llink, * nxt = del->next, * prv = del->prev;
    if (nxt) nxt->prev = prv;
    if (prv) prv->next = nxt;
    if (*list == del) *list = nxt;
    *llink = nxt;
    del->prev = NULL;
    del->next = NULL;
    return del;
}


/**

    gnui_lists_are_equal:
    @list1:     (not nullable): The first list
    @list2:     (not nullable): The second list

    Check if two `GLists` have the same elements in the same order

    Returns:    `true` if the two lists are equal, `false` otherwise

**/
static inline bool gnui_lists_are_equal (
    const GList * const list1,
    const GList * const list2
) {
    const GList * llnk1, * llnk2;
    for (
        llnk1 = list1, llnk2 = list2;
            llnk1 && llnk2 && llnk1->data == llnk2->data;
        llnk1 = llnk1->next, llnk2 = llnk2->next
    );
    return !llnk1 && !llnk2;
}


/**

    gnui_signal_handlers_block_by_func:
    @instance:  (not nullable): The `GObject` instance
    @func:      (not nullable): The event handler to block
    @data:      (nullable): The closure data of the handlers' closures

    Like `g_signal_handlers_block_by_func()`, but suppresses the
    function-pointer-to-object-pointer cast warning

    @return     The number of handlers that matched

**/
static inline guint gnui_signal_handlers_block_by_func (
    const gpointer instance,
    const GCallback func,
    const gpointer data
) {
    return g_signal_handlers_block_by_func(
        instance,
        *((gpointer *) &func),
        data
    );
}


/**

    gnui_signal_handlers_unblock_by_func:
    @instance:  (not nullable): The `GObject` instance
    @func:      (not nullable): The event handler to unblock
    @data:      (nullable): The closure data of the handlers' closures

    Like `g_signal_handlers_unblock_by_func()`, but suppresses the
    function-pointer-to-object-pointer cast warning

**/
static inline guint gnui_signal_handlers_unblock_by_func (
    const gpointer instance,
    const GCallback func,
    const gpointer data
) {
    return g_signal_handlers_unblock_by_func(
        instance,
        *((gpointer *) &func),
        data
    );
}


G_END_DECLS


#endif


/*  EOF  */

