/*  -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-  */

/*\
|*|
|*| gnui-internals.h
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



#ifndef _GNUI_INTERNALS_H_
#define _GNUI_INTERNALS_H_


#include <gtk/gtk.h>
#include "gnui-definitions.h"


#ifdef ENABLE_NLS
#include <glib/gi18n-lib.h>
#endif



/*\
|*|
|*| BUILD SETTINGS
|*|
\*/


#ifndef GNUISANCE_CONST_BUILD_FLAG_C_UNIT
#define GNUISANCE_CONST_BUILD_FLAG_C_UNIT gnuisance
#endif


#ifndef GNUISANCE_CONST_BUILD_FLAG_CPP_UNIT
#define GNUISANCE_CONST_BUILD_FLAG_CPP_UNIT GNUISANCE
#endif


#if defined(GNUISANCE_CONST_BUILD_FLAG_CPP_UNIT) && ( \
	GNUI_PP_IS_BLANK( \
		GNUI_PP_PASTE3(GNUISANCE_CONST_BUILD_FLAG_CPP_UNIT, _, BUILD_FLAG_MANUAL_ENVIRONMENT) \
	) || \
	GNUI_PP_BOOL( \
		GNUI_PP_PASTE3(GNUISANCE_CONST_BUILD_FLAG_CPP_UNIT, _, BUILD_FLAG_MANUAL_ENVIRONMENT) \
	) == 0 \
)
#define GNUISANCE_MODULE_BUILD_FLAG_MANUAL_ENVIRONMENT_DEFINED 1
#elif !defined(GNUISANCE_CONST_BUILD_FLAG_CPP_UNIT)
#define GNUISANCE_MODULE_BUILD_FLAG_MANUAL_ENVIRONMENT_DEFINED 0
#else
#define GNUISANCE_MODULE_BUILD_FLAG_MANUAL_ENVIRONMENT_DEFINED \
	GNUI_PP_PASTE3(GNUISANCE_CONST_BUILD_FLAG_CPP_UNIT, _, BUILD_FLAG_MANUAL_ENVIRONMENT)
#endif


#define GNUI_ENVIRONMENT_INIT \
	GNUI_PP_PASTE2(GNUISANCE_CONST_BUILD_FLAG_C_UNIT, _environment_init)


#define GNUI_ENVIRONMENT_IS_INITIALIZED \
	GNUI_PP_PASTE2(GNUISANCE_CONST_BUILD_FLAG_C_UNIT, _environment_is_initialized)


#define GNUI_ENVIRONMENT_GET_IS_INITIALIZED \
	GNUI_PP_PASTE2(GNUISANCE_CONST_BUILD_FLAG_C_UNIT, _environment_get_is_initialized)


#if defined(GNUISANCE_CONST_BUILD_FLAG_NO_ENVIRONMENT) || \
    defined(GNUISANCE_BUILD_FLAG_MANUAL_ENVIRONMENT) || \
    GNUISANCE_MODULE_BUILD_FLAG_MANUAL_ENVIRONMENT_DEFINED
#define GNUI_MODULE_ENSURE_ENVIRONMENT
#else
#include "gnui-environment.h"
#ifndef GNUI_ENVIRONMENT_CONST_BUILD_FLAG_NO_INIT
extern void GNUI_ENVIRONMENT_INIT (void);
#define GNUI_MODULE_ENSURE_ENVIRONMENT GNUI_ENVIRONMENT_INIT();
#else
#define GNUI_MODULE_ENSURE_ENVIRONMENT
#endif
#endif



/*\
|*|
|*| INTERNATIONALIZATION
|*|
\*/


#ifdef ENABLE_NLS
#define I18N_INIT() \
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR)
#else
#define _(STRING) ((char *) (STRING))
#define g_dngettext(DOMAIN, STRING_1, STRING_2, NUM) \
	((NUM) > 1 ? (char *) (STRING_2) : (char *) (STRING_1))
#define I18N_INIT()
#endif


#define _0(STRING) ((char *) (STRING))



/*\
|*|
|*| MACHINERY SUGAR
|*|
\*/


#ifndef I_
#define I_(STRING) g_intern_static_string(STRING)
#endif


#endif


/*  EOF  */

