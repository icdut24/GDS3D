//  GDS3D, a program for viewing GDSII files in 3D.
//  Created by Jasper Velner and Michiel Soer, http://icd.el.utwente.nl
//  Copyright (C) 2013 IC-Design Group, University of Twente.
//
//  Based on gds2pov by Roger Light, http://atchoo.org/gds2pov/ / https://github.com/ralight/gds2pov
//  Copyright (C) 2004-2008 by Roger Light
//
/*
 * File: gdsparse.h
 * Author: Roger Light
 * Project: gdsto3d
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

#ifndef __GDSPARSE_H__
#define __GDSPARSE_H__


#include "process_cfg.h"
#include "gds_globals.h"
#include "gdsobject.h"
#include "gdsobjectlist.h"

class GDSParse
{
protected:
	char			*_libname;
	char			*_topcellname;

	int16_t			_currentlayer;
	float			_currentwidth;
	int16_t			_currentpathtype;
	gds_element_type	_currentelement;
	int16_t			_currenttexttype;
	int16_t			_currentpresentation;
	char			*_textstring;
	int16_t			_currentstrans;
	float			_currentangle;
	int16_t			_currentdatatype;
	float			_currentmag;
	float			_currentbgnextn;
	float			_currentendextn;

	char			*_sname;
	int16_t			_arrayrows, _arraycols;
	float			_units;
	float			_angle;
	FILE			*_iptr;
	FILE			*_optr;
	class GDSProcess	*_process;
	
	int16_t			_recordlen;

	/* Output options */
	bool			_allow_multiple_output;
	bool			_output_children_first;
	bool			_use_outfile;
	bool			_generate_process;

	/*
	** Both of these variables have fixed bounds because
	** they are not dependant on the GDS2 spec, not on the
	** file we are parsing.
	** There will never be more than 70 records.
	** The maximum layer and datatype are both defined as
	** 255, but but could be as high as 32,767 because of
	** the way they are stored (2 byte int). It might be worth
	** checking if they are greater than 255
	*/
	bool			_unsupported[70];
	bool			_layer_warning[257][257];

	long			_PathElements;
	long			_BoundaryElements;
	long			_BoxElements;
	long			_TextElements;
	long			_SRefElements;
	long			_ARefElements;

	
	class GDSObject		*_CurrentObject;

	/* gds_parse.h functions */
	void ParseHeader();
	void ParseLibName();
	void ParseSName();
	void ParseUnits();
	void ParseStrName();
	void ParseXY();
	void ParseXYPath();
	void ParseXYBoundary();
	void ParseSTrans();
    void AddSubstrate(char *topcell);

	void HandleSRef();
	void HandleARef();
	void HandleBoundary();
	void HandlePath();

	short GetBitArray();
	double GetEightByteReal();
	int32_t GetFourByteSignedInt();
	int16_t GetTwoByteSignedInt();
	char *GetAsciiString();

	void ReportUnsupported(const char *Name, enum RecordNumbers rn);
	
	bool ParseFile(char *topcell);

public:
	class GDSObjectList	*_Objects; // Move to protected later on

	GDSParse (class GDSProcess *process, bool generate_process);
	virtual ~GDSParse ();

	bool Parse(FILE *iptr, char *topcell);
	virtual class GDSObject *NewObject(char *Name) = 0;
	void Reload();

	class GDSProcess *GetProcess();
};

#endif // __GDSPARSE_H__

