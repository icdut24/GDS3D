//  GDS3D, a program for viewing GDSII files in 3D.
//  Created by Jasper Velner and Michiel Soer, http://icd.el.utwente.nl
//  Copyright (C) 2013 IC-Design Group, University of Twente.
//
//  Based on gds2pov by Roger Light, http://atchoo.org/gds2pov/ / https://github.com/ralight/gds2pov
//  Copyright (C) 2004-2008 by Roger Light
//
/*
 * File: gdstext.h
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

#ifndef __GDSTEXT_H__
#define __GDSTEXT_H__

#include "gdselements.h"
#include "process_cfg.h"

class GDSText
{
private:
	float			_X;
	float			_Y;
	float			_Z;
	float			_Mag;
	int			_VJust;
	int			_HJust;
	struct ProcessLayer	*_Layer;
	char			*_String;
	Transform		_Rotate;
	bool			_Flipped;

public:
	GDSText(float X, float Y, float Z, bool Flipped, float Mag, int VJust, int HJust, struct ProcessLayer *Layer);
	~GDSText();

	void SetString(const char *String);
	void SetRotation(float X, float Y, float Z);

	char *GetString();
	float GetX();
	float GetY();
	float GetZ();
	float GetRY();
	float GetMag();

	int GetVJust();
	int GetHJust();

	bool GetFlipped();

	struct ProcessLayer *GetLayer();
};

#endif // __GDSTEXT_H__
