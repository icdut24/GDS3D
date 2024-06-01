//  GDS3D, a program for viewing GDSII files in 3D.
//  Created by Jasper Velner and Michiel Soer, http://icd.el.utwente.nl
//  Copyright (C) 2012 IC-Design Group, University of Twente.
//
//  Based on gds2pov by Roger Light, http://atchoo.org/gds2pov/ / https://github.com/ralight/gds2pov
//  Copyright (C) 2004-2008 by Roger Light
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//  
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//  
//  You should have received a copy of the GNU Lesser General Public
//  License along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef __MAIN_H__
#define __MAIN_H__

#include "../gdsoglviewer/windowmanager.h"

class htime_X11 : public htime
{
public:
	struct timeval start;
};

class Wm_X11 : public WindowManager
{
private:
	Display *dpy;
	Window win;
	Atom wmDelete;
	GLXContext ctx;
	Cursor null_cursor;
	GLuint font;
	bool run;
	bool active;
	vector<htime*> timers;
	time_t low_date_time, high_date_time;

	EventKey translateKey(int key);

public:
	Wm_X11();
	~Wm_X11();

	void gl_finish();
	bool hide_mouse(void);
	bool show_mouse(void);
	void change_cursor(int shape);
	void move_mouse(int x, int y);
	GLuint get_font();
	float timer( htime *t, int reset );
	htime* new_timer();
	void render_text(int x, int y, const char * text, VECTOR4D color);
	bool query_update(FILE *f);

	int main(int argc, char *argv[]);
};

#endif