//  GDS3D, a program for viewing GDSII files in 3D.
//  Created by Jasper Velner and Michiel Soer, http://icd.el.utwente.nl
//  Copyright (C) 2013 IC-Design Group, University of Twente.
//
//  Based on gds2pov by Roger Light, http://atchoo.org/gds2pov/ / https://github.com/ralight/gds2pov
//  Copyright (C) 2004-2008 by Roger Light
//
/*
 * File: gds_globals.cpp
 * Author: Roger Light
 * Project: gdsto3d
 *
 * This file contains non-class functions.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */



#include "gds_globals.h"

// For the console output
#ifdef WIN32
	#include <Windows.h>
	#include <WinBase.h>
#endif

int verbose_output=1; // Is this also initialized elsewhere?

void v_printf(const int level, const char *fmt, ...)
{
	if(verbose_output>=level){
		va_list va;
		va_start(va, fmt);
		
#ifndef WIN32
#ifndef __APPLE__
		if(level < 0) // Fatal error to stderr, do only for linux
			vfprintf(stderr, fmt, va);
		else
#endif
#endif
			vprintf(fmt, va);

#ifdef WIN32
		char buf[1024];
		vsprintf(buf, fmt, va);

		int len = strlen(buf)+1;
		wchar_t *wText = new wchar_t[len];
		if ( wText == 0 )
			return;
		memset(wText,0,len);
		::MultiByteToWideChar(  CP_ACP, NULL,buf, -1, wText,len );


		OutputDebugString(wText);
#endif
		va_end(va);
	}
}

