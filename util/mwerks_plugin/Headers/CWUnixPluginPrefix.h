/*
 *	CWUnixPluginPrefix.h
 *
 *	Copyright (c) 1999 Metrowerks, Inc.  All rights reserved.
 *
 */
 
#ifndef MW_CWUnixPluginPrefix_H
#define MW_CWUnixPluginPrefix_H

#define CW_USE_PRAGMA_EXPORT 0

#include "CWRuntimeFeatures.h"

#ifdef __MWERKS__
	#if !__option(bool)
		#ifndef true
			#define true 1
		#endif
		#ifndef false
			#define false 0
		#endif
	#endif
#endif


#endif /* MW_CWUnixPluginPrefix_H */
