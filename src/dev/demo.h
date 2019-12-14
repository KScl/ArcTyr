/** OpenTyrian - Arcade Version
 *
 * Copyright          (C) 2007-2019  The OpenTyrian Development Team
 * Portions copyright (C) 2019       Kaito Sinclaire
 *
 * This program is free software distributed under the
 * terms of the GNU General Public License, version 2.
 * See the 'COPYING' file for further details.
 */
/// \file  dev/demo.h
/// \brief Demo recorder devtool

#ifndef DEV_DEMO_H
#define DEV_DEMO_H

#include "../opentyr.h"

#ifdef ENABLE_DEVTOOLS
void DEV_RecordDemoInit( void );
void DEV_RecordDemoStart( void );
void DEV_RecordDemoInput( void );
void DEV_RecordDemoEnd( void );
#else
#define DEV_RecordDemoInit() (void)false
#define DEV_RecordDemoStart() (void)false
#define DEV_RecordDemoInput() (void)false
#define DEV_RecordDemoEnd() (void)false
#endif

#endif /* DEV_DEMO_H */
