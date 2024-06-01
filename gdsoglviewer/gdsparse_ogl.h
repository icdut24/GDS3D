//  GDS3D, a program for viewing GDSII files in 3D.
//  Created by Jasper Velner and Michiel Soer, http://icd.el.utwente.nl
//  Copyright (C) 2013 IC-Design Group, University of Twente.
//
//  Based on gds2pov by Roger Light, http://atchoo.org/gds2pov/ / https://github.com/ralight/gds2pov
//  Copyright (C) 2004-2008 by Roger Light
//
/*
 * File: gdsparse_ogl.h
 * Author: Roger Light
 * Project: gdsto3d
 *
  
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __GDSPARSE_OGL_H__
#define __GDSPARSE_OGL_H__

#include "process_cfg.h"
#include "gdsparse.h"
#include "listview.h"
#include "gdsobject_ogl.h"
#include "ui_element.h"

class GDSParse_ogl : public GDSParse
{
private:
	bool _perfmon;
	bool _turbo;
	int _frames;
	bool firstrun;

	// Camera -> put in its own class
	GLfloat drawfps;
	float tt;
	float _fps;
	GLfloat _rx, _ry;
	GLfloat _vrx, _vry;
	GLfloat _vx, _vy, _vz; // Constant movement
	GLfloat _vx2, _vy2, _vz2; // Smooth declining movement
	GLfloat _zfar;	
	GLfloat _speed_factor; /* to have a similar speed independent of the feature size */

    renderRecipe_t  *substrate;
    ProcessLayer    *sub_layer;

	htime *_tv, *_mt; // Move to window manager
    
    // UI elements
    UIElement *ui_ruler;
	UIElement *ui_highlight;
    list<UIElement*> ui_elements;

public:
	GLfloat _xmin,_xmax,_ymin,_ymax; /* the bounding box for the gds features, FIXME: include z values */
	GLfloat _x, _y, _z;

	// Mouse control
	bool _mouse_control; // Left button
	bool _mouse_control2; // Right button
	bool _first_move;
	int mouse_prev_x;
	int mouse_prev_y;
	bool _temp_mouse;
	GDSObject_ogl*      _topcell;

	GDSParse_ogl (class GDSProcess *process, bool generate_process);
	~GDSParse_ogl ();

	class GDSObject *NewObject(char *Name);

	int SetTopcell(const char *topcell);
    void initWorld();
	void buildSubstrate();
	int gl_init();
	void gl_drawloading();
	void gl_drawcapturing();
	void gl_draw();
	void gl_draw_world(int width, int height, bool HQ);
	void gl_printf( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha, GLint x, GLint y, const char *format, ... );
	void gl_event( int event, int data, int xpos, int ypos , bool shift = false, bool control = false, bool alt=false);
	int gl_main(int fullscreen);

	void display_perfmon();
	void gl_square(float x1, float x2, float x3, float x4, int filled);
	void init_viewposition();

	void LockOnUIElement(UIElement* lock);
};

extern MATRIX4X4 worldview; // Please make a camera object and move this there
extern MATRIX4X4 projection;

#endif // __GDSPARSE_OGL_H__


