//  GDS3D, a program for viewing GDSII files in 3D.
//  Created by Jasper Velner and Michiel Soer, http://icd.el.utwente.nl
//  Copyright (C) 2013 IC-Design Group, University of Twente.
//
//  Based on gds2pov by Roger Light, http://atchoo.org/gds2pov/ / https://github.com/ralight/gds2pov
//  Copyright (C) 2004-2008 by Roger Light
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//  
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//  
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef __GDSELEMENTS_H__
#define __GDSELEMENTS_H__

class Point2D
{
public:
	float X;
	float Y;

	Point2D(){};
	Point2D(float X, float Y){this->X = X; this->Y = Y;};
};


typedef struct Transform{
	float X;
	float Y;
	float Z;
} Transform;

typedef struct ElementColour{
	float R;
	float G;
	float B;
	float F;
	int Metal;
} ElementColour;

typedef struct SRefElement {
    bool collapsed;
	float X;
	float Y;
	float Mag;
	char *Name;
	Transform Rotate;
	int Flipped;

	class GDSObject *object;
} SRefElement;

typedef struct ARefElement {
    bool collapsed;
	float X1;
	float Y1;
	float X2;
	float Y2;
	float X3;
	float Y3;
	float Mag;
	int Columns;
	int Rows;
	char *Name;
	Transform Rotate;
	int Flipped;

	class GDSObject *object;
} ARefElement;

#endif // __GDSELEMENTS_H__
