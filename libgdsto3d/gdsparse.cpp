//  GDS3D, a program for viewing GDSII files in 3D.
//  Created by Jasper Velner and Michiel Soer, http://icd.el.utwente.nl
//  Copyright (C) 2013 IC-Design Group, University of Twente.
//
//  Based on gds2pov by Roger Light, http://atchoo.org/gds2pov/ / https://github.com/ralight/gds2pov
//  Copyright (C) 2004-2008 by Roger Light
//
/*
 * File: gdsparse.cpp
 * Author: Roger Light
 * Project: gdsto3d
 *
 * This is the GDSParse class which is used to parse a GDS file and create a
 * GDSObjects object containing the contents of the file according to the 
 * process configuration.
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

#include "gdsparse.h"
#include "../math/Maths.h"

extern int verbose_output;

#ifndef endian_swap_long
    #define endian_swap_long(w) ( ((w&0xff)<<24) | ((w&0xff00)<<8) | ((w&0xff0000)>>8) | ((w&0xff000000)>>24) )
#endif
#ifndef endian_swap_short
    #define endian_swap_short(w) ( ((w&0xff)<<8) | ((w&0xff00)>>8) )
#endif

GDSParse::GDSParse (class GDSProcess *process, bool generate_process)
{
	_iptr = NULL;
	_optr = NULL;
	_libname = NULL;
	_sname = NULL;
	_textstring = NULL;
	_Objects = NULL;

	_PathElements = 0;
	_BoundaryElements = 0;
	_BoxElements = 0;
	_TextElements = 0;
	_SRefElements = 0;
	_ARefElements = 0;

	_currentangle = 0.0;
	_currentwidth = 0.0;
	_currentstrans = 0;
	_currentpathtype = 0;
	_currentlayer = -1;
	_currentdatatype = -1;
	_currentmag = 1.0;
	_currentbgnextn = 0.0;
	_currentendextn = 0.0;
	_currenttexttype = 0;
	_currentpresentation = 0;
//	_currentelement = 0;
	_arrayrows = 0;
	_arraycols = 0;
	_angle = 0.0;
	_units = 0.0;
	_recordlen = 0;
	_CurrentObject = NULL;

	_use_outfile = false;
	_allow_multiple_output = false;
	_output_children_first = false;
	_generate_process = generate_process;

	_process = process;

	for(int i=0; i<70; i++){
		_unsupported[i] = false;
	}
	for(int i=0; i<257; i++){
		for(int j=0; j<257; j++){
			_layer_warning[i][j] = false;
		}
	}
}

GDSParse::~GDSParse ()
{
	if(_libname){
		delete [] _libname;
	}
	if(_sname){
		delete [] _sname;
	}
	if(_textstring){
		delete [] _textstring;
	}
	if(_Objects){
		delete _Objects;
	}
}

bool GDSParse::Parse(FILE *iptr, char *topcell)
{
	_iptr = iptr;
	if(_iptr){
		_Objects = new GDSObjectList;

		bool result = ParseFile(topcell);

		v_printf(1, "\nSummary:\n\tPaths:\t\t%ld\n\tBoundaries:\t%ld\n\tBoxes:\t\t%ld\n\tStrings:\t%ld\n\tStuctures:\t%ld\n\tArrays:\t\t%ld\n\n",
			_PathElements, _BoundaryElements, _BoxElements, _TextElements, _SRefElements, _ARefElements);

		return result;
	}else{
		return true;
	}
}

void GDSParse::Reload()
{
	if(_Objects)
	{
		delete _Objects;
		_Objects = new GDSObjectList;
		ParseFile(this->_topcellname);
	}
}

bool GDSParse::ParseFile(char *topcell)
{
	byte recordtype, datatype;
	char *tempstr;
	struct ProcessLayer *layer = NULL;
    
    float BgnExtn;
    float EndExtn;
    float extn_x, extn_x2;
    float extn_y, extn_y2;
    float angleX[1024]; // HACK
    float angleY[1024]; // HACK
    Point2D points[8];

	this->_topcellname = topcell;
    _currentelement = elNone;

	if(!_iptr){
		return true;
	}

	fseek(_iptr, 0, SEEK_SET);
	while(!feof(_iptr)){
		_recordlen = GetTwoByteSignedInt();
		fread(&recordtype, 1, 1, _iptr);
		fread(&datatype, 1, 1, _iptr);
		_recordlen -= 4;
		switch(recordtype){
			case rnHeader:
				v_printf(3, "HEADER\n");
				ParseHeader();
				break;
			case rnBgnLib:
				v_printf(3, "BGNLIB\n");
				while(_recordlen){
					GetTwoByteSignedInt();
				}
				break;
			case rnLibName:
				v_printf(3, "LIBNAME ");
				ParseLibName();
				break;
			case rnUnits:
				v_printf(3, "UNITS\n");
				ParseUnits();
				break;
			case rnEndLib:
				v_printf(3, "ENDLIB\n");
				fseek(_iptr, 0, SEEK_END);
        //Added for substrate
      	_BoundaryElements++;
        _currentlayer = 255;
        _currentdatatype = 0;
        AddSubstrate(topcell);
                _Objects->ConnectReferences();

				return 0;
				break;
			case rnEndStr:
				v_printf(3, "ENDSTR\n");				

				// Reset transformation matrix
				_currentstrans = 0;
				break;
			case rnEndEl:
				v_printf(3, "ENDEL\n\n");
                
                // End of path, text or boundary
                switch(_currentelement){
                    case elBoundary:
                        if(_CurrentObject)
                        {
                            if(_CurrentObject->GetCurrentPolygon())
							{
                                _CurrentObject->GetCurrentPolygon()->Orientate(); // Check normal pointing
								_CurrentObject->GetCurrentPolygon()->Tesselate();
							}
                        }
                        break;
					case elPath:
                        
                        if(_CurrentObject)
                        {
                            if(_CurrentObject->GetCurrentPath())
                            {
                                GDSPath *path;
                                path = _CurrentObject->GetCurrentPath();
                                _CurrentObject->AddPolygon(path->GetHeight(), path->GetThickness(), path->GetPoints()*2, path->GetLayer());
                                GDSPolygon* poly = _CurrentObject->GetCurrentPolygon();
                                
                                ProcessLayer *layer = path->GetLayer();
                                
                                if(path->GetWidth() && layer){
                                    switch(path->GetType()){
                                        case 1:
                                        case 2:
                                            BgnExtn = path->GetWidth(); /* Width has already been scaled to half */
                                            EndExtn = path->GetWidth();
                                            break;
                                        case 4:
                                            BgnExtn = path->GetBgnExtn();
                                            EndExtn = path->GetEndExtn();
                                            break;
                                        default:
                                            BgnExtn = 0.0;
                                            EndExtn = 0.0;
                                            break;
                                    }
                                    
                                    float dx, dy;
                                    // Cache angles
                                    for(unsigned long j=0; j<path->GetPoints()-1; j++){
                                        dx = path->GetXCoords(j) - path->GetXCoords(j+1);
                                        dy = path->GetYCoords(j+1) - path->GetYCoords(j);
                                        
                                        /* This is only run once each time the program is run, so we can more afford the expensive trig functions.
                                         Sort the above out properly later. */
                                        angleX[j] = cos(atan2(dx, dy));
                                        angleY[j] = sin(atan2(dx, dy));
                                    }
                                    
                                    // Make path
                                    float angleX_1;
                                    float angleY_1;
                                    float angleX_2;
                                    float angleY_2;
                                    float l; // Normalization
                                    Point2D P[256];
                                    
									extn_x = extn_y = extn_x2 = extn_y2 = 0.0;
                                    for(unsigned long j=0; j<path->GetPoints()-1; j++){
                                        if(j==0){
                                            extn_x = BgnExtn * angleY[j];
                                            extn_y = BgnExtn * angleX[j];
											extn_x2 = EndExtn * angleY[j];
                                            extn_y2 = EndExtn * angleX[j];
                                            angleX_1 = angleX[j]; 
                                            angleY_1 = angleY[j];
                                            if(path->GetPoints()>2){
                                                angleX_2 = (angleX[j]+angleX[j+1])/2;
                                                angleY_2 = (angleY[j]+angleY[j+1])/2;
                                            }
                                            else{
                                                angleX_2 = angleX[j]; 
                                                angleY_2 = angleY[j];
                                            }
                                            
                                        }
                                        else if(j==path->GetPoints()-2){
											extn_x = BgnExtn * angleY[j];
                                            extn_y = BgnExtn * angleX[j];
                                            extn_x2 = EndExtn * angleY[j];
                                            extn_y2 = EndExtn * angleX[j];
                                            angleX_1 = (angleX[j]+angleX[j-1])/2;
                                            angleY_1 = (angleY[j]+angleY[j-1])/2;
                                            angleX_2 = angleX[j]; 
                                            angleY_2 = angleY[j];
                                        }else{
											extn_x = extn_y = extn_x2 = extn_y2 = 0.0;
                                            //extn_x = 0.0;
                                            //extn_y = 0.0;
                                            angleX_1 = (angleX[j]+angleX[j-1])/2;
                                            angleY_1 = (angleY[j]+angleY[j-1])/2;
                                            angleX_2 = (angleX[j]+angleX[j+1])/2;
                                            angleY_2 = (angleY[j]+angleY[j+1])/2;
                                        }
                                        
                                        // Normalize
                                        l = sqrt(angleX_1*angleX_1+angleY_1*angleY_1);
                                        if(l>0.01){
                                            angleX_1 /= l; angleY_1 /= l;
                                        }
                                        l = sqrt(angleX_2*angleX_2+angleY_2*angleY_2);
                                        if(l>0.01){
                                            angleX_2 /= l; angleY_2 /= l;
                                        }
                                        
                                        // Readjust path width
                                        l = (angleX_1*angleX[j]+angleY_1*angleY[j]);
                                        if(l>0.01){
                                            angleX_1 /= l; angleY_1 /= l;
                                        }
                                        l = (angleX_2*angleX[j]+angleY_2*angleY[j]);
                                        if(l>0.01){
                                            angleX_2 /= l; angleY_2 /= l;
                                        }
                                        
                                        // 1
                                        if(j==0){
                                            points[0].X = path->GetXCoords(j) - path->GetWidth() * angleX_1 + extn_x;// + offx;
                                            points[0].Y = path->GetYCoords(j) - path->GetWidth() * angleY_1 - extn_y;// + offy;
                                        }
                                        else{
                                            points[0].X = path->GetXCoords(j) - path->GetWidth() * angleX_1;// + offx;
                                            points[0].Y = path->GetYCoords(j) - path->GetWidth() * angleY_1;// + offy;
                                        }
                                        //points[0].Z = z2;
                                        
                                        // 2
                                        if(j==0){
                                            points[1].X = path->GetXCoords(j) + path->GetWidth() * angleX_1 + extn_x;// + offx;
                                            points[1].Y = path->GetYCoords(j) + path->GetWidth() * angleY_1 - extn_y;// + offy;
                                        }
                                        else{
                                            points[1].X = path->GetXCoords(j) + path->GetWidth() * angleX_1; 
                                            points[1].Y = path->GetYCoords(j) + path->GetWidth() * angleY_1;
                                        }
                                        //points[1].Z = z2;
                                        
                                        // 3 
                                        if(j==path->GetPoints()-2){
                                            points[2].X = path->GetXCoords(j+1) + path->GetWidth() * angleX_2 - extn_x2;// + offx;
                                            points[2].Y = path->GetYCoords(j+1) + path->GetWidth() * angleY_2 + extn_y2;// + offy;
                                        }
                                        else{
                                            points[2].X = path->GetXCoords(j+1) + path->GetWidth() * angleX_2;// + offx;
                                            points[2].Y = path->GetYCoords(j+1) + path->GetWidth() * angleY_2;// + offy;
                                        }
                                        //points[2].Z = z2;
                                        
                                        // 4
                                        if(j==path->GetPoints()-2){
                                            points[3].X = path->GetXCoords(j+1) - path->GetWidth() * angleX_2 - extn_x2;// + offx;
                                            points[3].Y = path->GetYCoords(j+1) - path->GetWidth() * angleY_2 + extn_y2;// + offy;
                                        }
                                        else{
                                            points[3].X = path->GetXCoords(j+1) - path->GetWidth() * angleX_2;// + offx;
                                            points[3].Y = path->GetYCoords(j+1) - path->GetWidth() * angleY_2;// + offy;
                                        }
                                        //points[3].Z = z2;
                                        
                                        
                                        if(j==0)
                                        {
                                            P[0].X = points[0].X; P[0].Y = points[0].Y;
                                            P[1].X = points[1].X; P[1].Y = points[1].Y;
                                            
                                            //poly->AddPoint(points[0].X, points[0].Y);
                                            //poly->AddPoint(points[1].X, points[1].Y);
                                            //poly->AddPoint(poly->GetPoints()-1, points[0].X, points[0].Y); // Close contour
                                        }
                                        P[j+2].X = points[2].X; P[j+2].Y = points[2].Y;
                                        P[path->GetPoints()*2-1-j].X = points[3].X; P[path->GetPoints()*2-1-j].Y = points[3].Y;
                                        //poly->AddPoint(points[2].X, points[2].Y);
                                        //verts[j+2] = points[2];
                                        //poly->AddPoint(points[3].X, points[3].Y);
                                        //verts[numVerts-1-j] = points[3];
                                    }

                                    poly->Clear();
                                    for(unsigned j=0;j<path->GetPoints()*2;j++)
                                        poly->AddPoint(P[j].X, P[j].Y);
									poly->Tesselate();
                                    
                                }
                            }
                        }
                        break;
                        
                    default:
                        break;
                }
				_currentstrans = 0;
				break;
			case rnBgnStr:
				v_printf(3, "BGNSTR\n");
				while(_recordlen){
					GetTwoByteSignedInt();
				}
				break;
			case rnStrName:
				v_printf(3, "STRNAME ");
				ParseStrName();
				break;
			case rnBoundary:
				v_printf(3, "BOUNDARY ");
				_currentelement = elBoundary;
				break;
			case rnPath:
				v_printf(3, "PATH ");
				_currentelement = elPath;
				break;
			case rnSRef:
				v_printf(3, "SREF ");
				_currentelement = elSRef;
				break;
			case rnARef:
				v_printf(3, "AREF ");
				_currentelement = elARef;
				break;
			case rnText:
				v_printf(3, "TEXT ");
				_currentelement = elText;
				break;
			case rnLayer:
				_currentlayer = GetTwoByteSignedInt();
				v_printf(3, "LAYER (%d)\n", _currentlayer);
				break;
			case rnDataType:
				_currentdatatype = GetTwoByteSignedInt();
				v_printf(3, "DATATYPE (%d)\n", _currentdatatype);
				break;
			case rnWidth:
				_currentwidth = (float)(GetFourByteSignedInt()/2);
				if(_currentwidth > 0){
					_currentwidth *= _units;
				}
				v_printf(3, "WIDTH (%.3f)\n", _currentwidth*2);
				// Scale to a half to make width correct when adding and
				// subtracting
				break;
			case rnXY:
				v_printf(3, "XY ");
				switch(_currentelement){
					case elBoundary:
						_BoundaryElements++;
						ParseXYBoundary();
						break;
					case elBox:
						_BoxElements++;
						ParseXYBoundary();
						break;
					case elPath:
						_PathElements++;
						ParseXYPath();
						break;
					default:
						ParseXY();
						break;
				}
				break;
			case rnColRow:
				_arraycols = GetTwoByteSignedInt();
				_arrayrows = GetTwoByteSignedInt();
				v_printf(3, "COLROW (Columns = %d Rows = %d)\n", _arraycols, _arrayrows);
				break;
			case rnSName:
				ParseSName();
				break;
			case rnPathType:
				if(!_unsupported[rnPathType]){
					v_printf(2, "Incomplete support for GDS2 record type: PATHTYPE\n");
					_unsupported[rnPathType] = true;
				}
				//FIXME
				_currentpathtype = GetTwoByteSignedInt();
				v_printf(3, "PATHTYPE (%d)\n", _currentpathtype);
				break;
			case rnTextType:
				ReportUnsupported("TEXTTYPE", rnTextType);
				_currenttexttype = GetTwoByteSignedInt();
				v_printf(3, "TEXTTYPE (%d)\n", _currenttexttype);
				break;
			case rnPresentation:
				_currentpresentation = GetTwoByteSignedInt();
				v_printf(3, "PRESENTATION (%d)\n", _currentpresentation);
				break;
			case rnString:
				v_printf(3, "STRING ");
				if(_textstring){
					delete [] _textstring;
					_textstring = NULL;
				}
				_textstring = GetAsciiString();
				/* Only set string if the current object is valid, the text string is valid 
				 * and we are using a layer that is defined and being shown.
				 */
				if(_CurrentObject && _CurrentObject->GetCurrentText() && _textstring){
					if(_process != NULL){
						layer = _process->GetLayer(_currentlayer, _currentdatatype);
						//if(layer && layer->Show){
							_CurrentObject->GetCurrentText()->SetString(_textstring);
						//}
					}else{
						_CurrentObject->GetCurrentText()->SetString(_textstring);
					}
					v_printf(3, "(\"%s\")", _textstring);
					delete [] _textstring;
					_textstring = NULL;
				}else if(!_textstring){
					return true;
				}
				v_printf(3, "\n");
				break;
			case rnSTrans:
				//if(!_unsupported[rnSTrans]){
				//	v_printf(1, "Incomplete support for GDS2 record type: STRANS\n");
				//	_unsupported[rnSTrans] = true;
				//}
				//Fixed by Silencer
				_currentstrans = GetTwoByteSignedInt();
				v_printf(3, "STRANS (%d)\n", _currentstrans);
				break;
			case rnMag:
				_currentmag = (float) GetEightByteReal();
				v_printf(3, "MAG (%f)\n", _currentmag);
				break;
			case rnAngle:
				_currentangle = (float)GetEightByteReal();
				v_printf(3, "ANGLE (%f)\n", _currentangle);
				break;
/*			case rnUInteger:
				break;
Not used in GDS2 spec	case rnUString:
				break;
*/
			case rnRefLibs:
				ReportUnsupported("REFLIBS", rnRefLibs);
				tempstr = GetAsciiString();
				v_printf(3, "REFLIBS (\"%s\")\n", tempstr);
				delete [] tempstr;
				break;
			case rnFonts:
				ReportUnsupported("FONTS", rnFonts);
				tempstr = GetAsciiString();
				v_printf(3, "FONTS (\"%s\")\n", tempstr);
				delete [] tempstr;
				break;
			case rnGenerations:
				ReportUnsupported("GENERATIONS", rnGenerations);
				v_printf(3, "GENERATIONS\n");
				v_printf(3, "\t");
				while(_recordlen){
					v_printf(3, "%d ", GetTwoByteSignedInt());
				}
				v_printf(3, "\n");
				break;
			case rnAttrTable:
				ReportUnsupported("ATTRTABLE", rnAttrTable);
				tempstr = GetAsciiString();
				v_printf(3, "ATTRTABLE (\"%s\")\n", tempstr);
				delete [] tempstr;
				break;
			case rnStypTable:
				ReportUnsupported("STYPTABLE", rnStypTable);
				v_printf(3, "STYPTABLE (\"%d\")\n", GetTwoByteSignedInt());
				break;
			case rnStrType:
				ReportUnsupported("STRTYPE", rnStrType);
				tempstr = GetAsciiString();
				v_printf(3, "STRTYPE (\"%s\")\n", tempstr);
				delete [] tempstr;
				break;
			case rnElFlags:
				ReportUnsupported("ELFLAGS", rnElFlags);
				v_printf(3, "ELFLAGS (");
				while(_recordlen){
					v_printf(3, "%d ", GetTwoByteSignedInt());
				}
				v_printf(3, ")\n");
				break;
			case rnElKey:
				ReportUnsupported("ELKEY", rnElKey);
				v_printf(3, "ELKEY (");
				while(_recordlen){
					v_printf(3, "%d ", GetTwoByteSignedInt());
				}
				v_printf(3, ")\n");
				break;
			case rnLinkType:
				ReportUnsupported("LINKTYPE", rnLinkType);
				v_printf(3, "LINKTYPE (");
				while(_recordlen){
					v_printf(3, "%d ", GetTwoByteSignedInt());
				}
				v_printf(3, ")\n");
				break;
			case rnLinkKeys:
				ReportUnsupported("LINKKEYS", rnLinkKeys);
				v_printf(3, "LINKKEYS (");
				while(_recordlen){
					v_printf(3, "%ld ", GetFourByteSignedInt());
				}
				v_printf(3, ")\n");
				break;
			case rnNodeType:
				ReportUnsupported("NODETYPE", rnNodeType);
				v_printf(3, "NODETYPE (");
				while(_recordlen){
					v_printf(3, "%d ", GetTwoByteSignedInt());
				}
				v_printf(3, ")\n");
				break;
			case rnPropAttr:
				ReportUnsupported("PROPATTR", rnPropAttr);
				v_printf(3, "PROPATTR (");
				while(_recordlen){
					v_printf(3, "%d ", GetTwoByteSignedInt());
				}
				v_printf(3, ")\n");
				break;
			case rnPropValue:
				ReportUnsupported("PROPVALUE", rnPropValue);
				tempstr = GetAsciiString();
				v_printf(3, "PROPVALUE (\"%s\")\n", tempstr);
				delete [] tempstr;
				break;
			case rnBox:
				ReportUnsupported("BOX", rnBox);
				v_printf(3, "BOX\n");
				/* Empty */
				_currentelement = elBox;
				break;
			case rnBoxType:
				ReportUnsupported("BOXTYPE", rnBoxType);
				v_printf(3, "BOXTYPE (%d)\n", GetTwoByteSignedInt());
				break;
			case rnPlex:
				ReportUnsupported("PLEX", rnPlex);
				v_printf(3, "PLEX (");
				while(_recordlen){
					v_printf(3, "%ld ", GetFourByteSignedInt());
				}
				v_printf(3, ")\n");
				break;
			case rnBgnExtn:
				ReportUnsupported("BGNEXTN", rnBgnExtn);
				_currentbgnextn = _units * (float)GetFourByteSignedInt();
				v_printf(3, "BGNEXTN (%f)\n", _currentbgnextn);
				break;
			case rnEndExtn:
				ReportUnsupported("ENDEXTN", rnEndExtn);
				_currentendextn = _units * (float)GetFourByteSignedInt();
				v_printf(3, "ENDEXTN (%ld)\n", _currentendextn);
				break;
			case rnTapeNum:
				ReportUnsupported("TAPENUM", rnTapeNum);
				v_printf(3, "TAPENUM\n");
				v_printf(3, "\t");
				while(_recordlen){
					v_printf(3, "%d ", GetTwoByteSignedInt());
				}
				v_printf(3, "\n");
				break;
			case rnTapeCode:
				ReportUnsupported("TAPECODE", rnTapeCode);
				v_printf(3, "TAPECODE\n");
				v_printf(3, "\t");
				while(_recordlen){
					v_printf(3, "%d ", GetTwoByteSignedInt());
				}
				v_printf(3, "\n");
				break;
			case rnStrClass:
				ReportUnsupported("STRCLASS", rnStrClass);
				v_printf(3, "STRCLASS (");
				while(_recordlen){
					v_printf(3, "%d ", GetTwoByteSignedInt());
				}
				v_printf(3, ")\n");
				break;
			case rnReserved:
				ReportUnsupported("RESERVED", rnReserved);
				v_printf(3, "RESERVED\n");
				/* Empty */
				break;
			case rnFormat:
				ReportUnsupported("FORMAT", rnFormat);
				v_printf(3, "FORMAT (");
				while(_recordlen){
					v_printf(3, "%d ", GetTwoByteSignedInt());
				}
				v_printf(3, ")\n");
				break;
			case rnMask:
				ReportUnsupported("MASK", rnMask);
				tempstr = GetAsciiString();
				v_printf(3, "MASK (\"%s\")\n", tempstr);
				delete [] tempstr;
				break;
			case rnEndMasks:
				ReportUnsupported("ENDMASKS", rnEndMasks);
				v_printf(3, "ENDMASKS\n");
				/* Empty */
				break;
			case rnLibDirSize:
				ReportUnsupported("LIBDIRSIZE", rnLibDirSize);
				v_printf(3, "LIBDIRSIZE (");
				while(_recordlen){
					v_printf(3, "%d ", GetTwoByteSignedInt());
				}
				v_printf(3, ")\n");
				break;
			case rnSrfName:
				ReportUnsupported("SRFNAME", rnSrfName);
				tempstr = GetAsciiString();
				v_printf(3, "SRFNAME (\"%s\")\n", tempstr);
				delete [] tempstr;
				break;
			case rnLibSecur:
				ReportUnsupported("LIBSECUR", rnLibSecur);
				v_printf(3, "LIBSECUR (");
				while(_recordlen){
					v_printf(3, "%d ", GetTwoByteSignedInt());
				}
				v_printf(3, ")\n");
				break;
			case rnBorder:
				ReportUnsupported("BORDER", rnBorder);
				v_printf(3, "BORDER\n");
				/* Empty */
				break;
			case rnSoftFence:
				ReportUnsupported("SOFTFENCE", rnSoftFence);
				v_printf(3, "SOFTFENCE\n");
				/* Empty */
				break;
			case rnHardFence:
				ReportUnsupported("HARDFENCE", rnHardFence);
				v_printf(3, "HARDFENCE\n");
				/* Empty */
				break;
			case rnSoftWire:
				ReportUnsupported("SOFTWIRE", rnSoftWire);
				v_printf(3, "SOFTWIRE\n");
				/* Empty */
				break;
			case rnHardWire:
				ReportUnsupported("HARDWIRE", rnHardWire);
				v_printf(3, "HARDWIRE\n");
				/* Empty */
				break;
			case rnPathPort:
				ReportUnsupported("PATHPORT", rnPathPort);
				v_printf(3, "PATHPORT\n");
				/* Empty */
				break;
			case rnNodePort:
				ReportUnsupported("NODEPORT", rnNodePort);
				v_printf(3, "NODEPORT\n");
				/* Empty */
				break;
			case rnUserConstraint:
				ReportUnsupported("USERCONSTRAINT", rnUserConstraint);
				v_printf(3, "USERCONSTRAINT\n");
				/* Empty */
				break;
			case rnSpacerError:
				ReportUnsupported("SPACERERROR", rnSpacerError);
				v_printf(3, "SPACERERROR\n");
				/* Empty */
				break;
			case rnContact:
				ReportUnsupported("CONTACT", rnContact);
				v_printf(3, "CONTACT\n");
				/* Empty */
				break;
			default:
				v_printf(2, "Unknown record type (%d) at position %ld.\n", recordtype, ftell(_iptr));
				//return true;
				break;
		}
	}
	return 0;
}

void GDSParse::ParseHeader()
{
	short version;
	version = GetTwoByteSignedInt();
	v_printf(2, "Version = %d\n", version);
}

void GDSParse::ParseLibName()
{
	char *str;
	str = GetAsciiString();
	if(_libname){
		delete [] _libname;
		_libname = NULL;
	}
	_libname = new char[strlen(str)+1];
	if(_libname){
		strcpy(_libname, str);
		v_printf(3, " (\"%s\")\n", _libname);
	}else{
		v_printf(1, "\nUnable to allocate memory for string (%d)\n", strlen(str)+1);
	}
	delete [] str;
}

void GDSParse::ParseSName()
{
	v_printf(3, "SNAME ");

	char *str;
	str = GetAsciiString();
	if(_sname){
		delete [] _sname;
		_sname = NULL;
	}
	_sname = new char[strlen(str)+1];
	if(_sname){
		strcpy(_sname, str);
		for(unsigned int i=0; i<strlen(_sname); i++){
			if(_sname[i] && (_sname[i] < 48 || _sname[i] > 57) && (_sname[i] < 65 || _sname[i] > 90) && (_sname[i] < 97 || _sname[i] > 122)){
				_sname[i] = '_';
			}
		}
		v_printf(3, "(\"%s\")\n", _sname);
	}else{
		v_printf(1, "Unable to allocate memory for string (%d)\n", strlen(str)+1);
	}
	delete [] str;
}

void GDSParse::ParseUnits()
{
	double tmp;
	_units = (float)GetEightByteReal(); 
	tmp = GetEightByteReal();
	v_printf(2, "DB units/user units = %g\nSize of DB units in metres = %g\nSize of user units in m = %g\n\n", 1/_units, tmp, tmp/_units);
}

void GDSParse::ParseStrName()
{
	char *str=NULL;
	str = GetAsciiString();

	if(str){
		// Disallow invalid characters in POV-Ray _names.
		for(unsigned int i=0; i<strlen(str); i++){
			if(str[i] && (str[i] < 48 || str[i] > 57) && (str[i] < 65 || str[i] > 90) && (str[i] < 97 || str[i] > 122)){
				str[i] = '_';
			}
		}
		v_printf(3, "(\"%s\")", str);

		// This calls our own NewObject function which is pure virtual so the end 
		// user must define it. This means we can always add a unknown object as
		// long as it inherits from GDSObject.
		_CurrentObject = _Objects->AddObject(NewObject(str));
		delete [] str;
	}
	v_printf(3, "\n");
}

void GDSParse::ParseXYPath()
{
	float X, Y;
	int points = _recordlen/8;
	int i;
	struct ProcessLayer *thislayer = NULL;

	if(_process != NULL){
		thislayer = _process->GetLayer(_currentlayer, _currentdatatype);

		if(thislayer==NULL){
			// _layer_warning only has fixed bounds at the moment.
			// Not sure how to best make it dynamic.

			if(!_generate_process){
				if(!_layer_warning[_currentlayer+1][_currentdatatype+1]){
					v_printf(2, "Notice: Layer %d, datatype %d is in the GDS, but not in the process.\n", _currentlayer, _currentdatatype);
					//v_printf(1, "\tIgnoring this layer.\n");
					_layer_warning[_currentlayer+1][_currentdatatype+1] = true;
				}
			}else{
				if(!_layer_warning[_currentlayer+1][_currentdatatype+1]){
					_process->AddLayer(_currentlayer, _currentdatatype);
					_layer_warning[_currentlayer+1][_currentdatatype+1] = true;
				}
			}
			while(_recordlen){
				GetFourByteSignedInt();
			}
			_currentwidth = 0.0; // Always reset to default for paths in case width not specified
			_currentpathtype = 0;
			_currentangle = 0.0;
			_currentdatatype = -1;
			_currentmag = 1.0;

			_currentstrans=0; //Oh yeah!
			return;
		}
	}

	if(_currentwidth){
		/* FIXME - need to check for -ve value and then not scale */
		if(thislayer && thislayer->Thickness && _CurrentObject){
			_CurrentObject->AddPath(_currentpathtype, _units*thislayer->Height, _units*thislayer->Thickness, points, _currentwidth, _currentbgnextn, _currentendextn, thislayer);
		}
		for(i=0; i<points; i++){
			X = _units * (float)GetFourByteSignedInt();
			Y = _units * (float)GetFourByteSignedInt();
			v_printf(3, "(%.3f,%.3f) ", X, Y);
			if(thislayer && thislayer->Thickness &&  _CurrentObject){
				_CurrentObject->GetCurrentPath()->AddPoint(i, X, Y);
			}
		}
	}else{
		for(i=0; i<points; i++){
			GetFourByteSignedInt();
			GetFourByteSignedInt();
		}
	}
	v_printf(3, "\n");
	_currentwidth = 0.0; // Always reset to default for paths in case width not specified
	_currentpathtype = 0;
	_currentangle = 0.0;
	_currentdatatype = -1;
	_currentmag = 1.0;
	_currentbgnextn = 0.0;
	_currentendextn = 0.0;

_currentstrans=0; //Oh yeah!
}

void GDSParse::AddSubstrate(char *topcell)
{
	/*
  float X, Y;	
  int points = 4;
	int i;
	struct ProcessLayer *thislayer = NULL;

	if(_process != NULL){
		thislayer = _process->GetLayer(_currentlayer, _currentdatatype);

		if(thislayer==NULL){
			if(!_generate_process){
				if(_currentlayer == -1 || _currentdatatype == -1 || !_layer_warning[_currentlayer][_currentdatatype]){
					v_printf(1, "No substrate layer found in process file.\n");
					v_printf(1, "Add layer 255 to the process file to define a substrate layer.\n");
					_layer_warning[_currentlayer][_currentdatatype] = true;
				}
			}else{
				if(!_layer_warning[_currentlayer][_currentdatatype]){
					_process->AddLayer(_currentlayer, _currentdatatype);
					_layer_warning[_currentlayer][_currentdatatype] = true;
				}
			}
			_currentwidth = 0.0; // Always reset to default for paths in case width not specified
			_currentpathtype = 0;
			_currentangle = 0.0;
			_currentdatatype = -1;
			_currentmag = 1.0;
			
			_currentstrans=0; //Oh yeah!
			return;
		}
	}

	if(! _Objects->SearchObject(topcell)) 
		return; // Invalid topcell

	if(thislayer && thislayer->Thickness  && _CurrentObject && _Objects){
    //struct _Boundary *Boundary = _Objects->GetBoundary();

	v_printf(2, "\n");
	struct _Boundary *Boundary = _Objects->SearchObject(topcell)->GetBoundary(NULL);
    float XMin = Boundary->XMin;
    float YMin = Boundary->YMin;
    float XMax = Boundary->XMax;
    float YMax = Boundary->YMax;

	_topcell->AddPolygon(_units*thislayer->Height, _units*thislayer->Thickness, points, thislayer);

	float extra = 0.05f;
  	for(i=0; i<points; i++){
      switch(i) {
        case 0:
          X = XMin-(XMax-XMin)*extra;
          Y = YMin-(YMax-YMin)*extra;
          break;
        case 1:
          X = XMax+(XMax-XMin)*extra;
          Y = YMin-(YMax-YMin)*extra;
          break;
        case 2:
          X = XMax+(XMax-XMin)*extra;
          Y = YMax+(YMax-YMin)*extra;
          break;
        case 3:
          X = XMin-(XMax-XMin)*extra;
          Y = YMax+(YMax-YMin)*extra;
          break;
        default: // We will never use this, because we don't close the contour
          X = XMin-(XMax-XMin)*extra;
          Y = YMin-(YMax-YMin)*extra;
          break;
      }
     _Objects->SearchObject(topcell)->GetCurrentPolygon()->AddPoint(X, Y);
    }
	}
	_Objects->SearchObject(topcell)->GetCurrentPolygon()->Tesselate();
	v_printf(3, "\n");
	_currentwidth = 0.0; // Always reset to default for paths in case width not specified
	_currentpathtype = 0;
	_currentangle = 0.0;
	_currentdatatype = -1;
	_currentmag = 1.0;
	_currentbgnextn = 0.0;
	_currentendextn = 0.0;

_currentstrans=0; //Oh yeah!*/
}

void GDSParse::ParseXYBoundary()
{
	float X, Y;
	//float firstX=0.0, firstY=0.0;
	int points = _recordlen/8;
	int i;
	struct ProcessLayer *thislayer = NULL;

	if(_process != NULL){
		thislayer = _process->GetLayer(_currentlayer, _currentdatatype);

		if(thislayer==NULL){
			if(!_generate_process){
				if(!_layer_warning[_currentlayer+1][_currentdatatype+1]){
					v_printf(2, "Notice: Layer %d, datatype %d is in the GDS, but not in the process.\n", _currentlayer, _currentdatatype);
					//v_printf(1, "\tIgnoring this layer.\n");
					_layer_warning[_currentlayer+1][_currentdatatype+1] = true;
				}
			}else{
				if(!_layer_warning[_currentlayer+1][_currentdatatype+1]){
					_process->AddLayer(_currentlayer, _currentdatatype);
					_layer_warning[_currentlayer+1][_currentdatatype+1] = true;
				}
			}
			while(_recordlen){
				GetFourByteSignedInt();
			}
			_currentwidth = 0.0; // Always reset to default for paths in case width not specified
			_currentpathtype = 0;
			_currentangle = 0.0;
			_currentdatatype = -1;
			_currentmag = 1.0;

			_currentstrans=0; //Oh yeah!
			return;
		}
	}

	if(thislayer && thislayer->Thickness && _CurrentObject){
		_CurrentObject->AddPolygon(_units*thislayer->Height, _units*thislayer->Thickness, points-1, thislayer);
	}

	for(i=0; i<points; i++){
		X = _units * (float)GetFourByteSignedInt();
		Y = _units * (float)GetFourByteSignedInt();
		v_printf(3, "(%.3f,%.3f) ", X, Y);

		if(thislayer && thislayer->Thickness && _CurrentObject && i!=points-1){ // Don't close the contour!
			_CurrentObject->GetCurrentPolygon()->AddPoint(X, Y);
		}
	}
	v_printf(3, "\n");
	
	_currentwidth = 0.0; // Always reset to default for paths in case width not specified
	_currentpathtype = 0;
	_currentangle = 0.0;
	_currentdatatype = -1;
	_currentmag = 1.0;
	_currentbgnextn = 0.0;
	_currentendextn = 0.0;

_currentstrans=0; //Oh yeah!
}

void GDSParse::ParseXY()
{
	float X, Y;
	float firstX=0.0, firstY=0.0, secondX=0.0, secondY=0.0;
	struct ProcessLayer *thislayer = NULL;
	int Flipped;

	if(_process != NULL){
		thislayer = _process->GetLayer(_currentlayer, _currentdatatype);
	}
	Flipped = ((u_int16_t)(_currentstrans & 0x8000) == (u_int16_t)0x8000) ? 1 : 0;

	switch(_currentelement){
		case elSRef:
			_SRefElements++;
			X = _units * (float)GetFourByteSignedInt();
			Y = _units * (float)GetFourByteSignedInt();
			v_printf(3, "(%.3f,%.3f)\n", X, Y);

			if(_CurrentObject){
				_CurrentObject->AddSRef(_sname, X, Y, Flipped, _currentmag);
				if(_currentangle){
					_CurrentObject->SetSRefRotation(0, -_currentangle, 0);
				}
			}
			break;

		case elARef:
			_ARefElements++;
			firstX = _units * (float)GetFourByteSignedInt();
			firstY = _units * (float)GetFourByteSignedInt();
			secondX = _units * (float)GetFourByteSignedInt();
			secondY = _units * (float)GetFourByteSignedInt();
			X = _units * (float)GetFourByteSignedInt();
			Y = _units * (float)GetFourByteSignedInt();
			v_printf(3, "(%.3f,%.3f) ", firstX, firstY);
			v_printf(3, "(%.3f,%.3f) ", secondX, secondY);
			v_printf(3, "(%.3f,%.3f)\n", X, Y);

			if(_CurrentObject){
				
				_CurrentObject->AddARef(_sname, firstX, firstY, secondX, secondY, X, Y, _arraycols, _arrayrows, Flipped, _currentmag);
				if(_currentangle){
					_CurrentObject->SetARefRotation(0, -_currentangle, 0);
				}
			}
			break;

		case elText:
			_TextElements++;

			if(thislayer==NULL){
				if(!_generate_process)
				{
					if(!_layer_warning[_currentlayer+1][_currentdatatype+1]){
						v_printf(2, "Notice: Layer %d, datatype %d is in the GDS, but not in the process.\n", _currentlayer, _currentdatatype);
						//v_printf(1, "\tIgnoring this string.\n");
						_layer_warning[_currentlayer+1][_currentdatatype+1] = true;
					}
				}else{
					if(!_layer_warning[_currentlayer+1][_currentdatatype+1]){
						_process->AddLayer(_currentlayer, _currentdatatype);
						_layer_warning[_currentlayer+1][_currentdatatype+1] = true;
					}
				}
				while(_recordlen){
					GetFourByteSignedInt();
				}
				_currentwidth = 0.0; // Always reset to default for paths in case width not specified
				_currentpathtype = 0;
				_currentangle = 0.0;
				_currentdatatype = 0;
				_currentmag = 1.0;

				_currentstrans=0; //Oh yeah!
				return;
			}

			X = _units * (float)GetFourByteSignedInt();
			Y = _units * (float)GetFourByteSignedInt();
			v_printf(3, "(%.3f,%.3f)\n", X, Y);

			if(_CurrentObject && _CurrentObject->GetCurrentText()){
				int vert_just, horiz_just;

				vert_just = (((((unsigned long)_currentpresentation & 0x8 ) == (unsigned long)0x8 ) ? 2 : 0) + (((((unsigned long)_currentpresentation & 0x4 ) == (unsigned long)0x4 ) ? 1 : 0)));
				horiz_just = (((((unsigned long)_currentpresentation & 0x2 ) == (unsigned long)0x2 ) ? 2 : 0) + (((((unsigned long)_currentpresentation & 0x1 ) == (unsigned long)0x1 ) ? 1 : 0)));

				_CurrentObject->AddText(X, Y, _units*thislayer->Height, (bool) (Flipped==1), _currentmag, vert_just, horiz_just, thislayer);
				if(_currentangle){
					_CurrentObject->GetCurrentText()->SetRotation(0.0, -_currentangle, 0.0);
				}
			}
			break;
		default:
			while(_recordlen){
				GetFourByteSignedInt();
			}
			break;
	}
	_currentwidth = 0.0; // Always reset to default for paths in case width not specified
	_currentpathtype = 0;
	_currentangle = 0.0;
	_currentdatatype = -1;
	_currentmag = 1.0;
	_currentpresentation = 0;

_currentstrans=0; //Oh yeah!

}

short GDSParse::GetBitArray()
{
	byte byte1;

	fread(&byte1, 1, 1, _iptr);
	fread(&byte1, 1, 1, _iptr);

	_recordlen-=2;
	return 0;
}

double GDSParse::GetEightByteReal()
{
	byte value;
	byte b8, b2, b3, b4, b5, b6, b7;
	double sign=1.0;
	double exponent;
	double mant;

	fread(&value, 1, 1, _iptr);
	if(value & 128){
		value -= 128;
		sign = -1.0;
	}
	exponent = (double )value;
	exponent -= 64.0;
	mant=0.0;

	fread(&b2, 1, 1, _iptr);
	fread(&b3, 1, 1, _iptr);
	fread(&b4, 1, 1, _iptr);
	fread(&b5, 1, 1, _iptr);
	fread(&b6, 1, 1, _iptr);
	fread(&b7, 1, 1, _iptr);
	fread(&b8, 1, 1, _iptr);

	mant += b8;
	mant /= 256.0;
	mant += b7;
	mant /= 256.0;
	mant += b6;
	mant /= 256.0;
	mant += b5;
	mant /= 256.0;
	mant += b4;
	mant /= 256.0;
	mant += b3;
	mant /= 256.0;
	mant += b2;
	mant /= 256.0;

	_recordlen-=8;

	return sign*(mant*pow(16.0,exponent));
}

int32_t GDSParse::GetFourByteSignedInt()
{
	int32_t value;
	fread(&value, 4, 1, _iptr);
	
	_recordlen-=4;

#if __BYTE_ORDER == __LITTLE_ENDIAN
	return endian_swap_long(value);
#else
	return value;
#endif
}

int16_t GDSParse::GetTwoByteSignedInt()
{
	int16_t value;

	fread(&value, 2, 1, _iptr);

	_recordlen-=2;

#if __BYTE_ORDER == __LITTLE_ENDIAN
	return endian_swap_short(value);
#else
	return value;
#endif
}

char *GDSParse::GetAsciiString()
{
	char *str=NULL;
	
	if(_recordlen>0){
		_recordlen += _recordlen%2; /* Make sure length is even, why would you do this? */
		str = new char[_recordlen+1];
		if(!str){
			v_printf(1, "Unable to allocate memory for ascii string (%d)\n", _recordlen);
			return NULL;
		}
		fread(str, 1, _recordlen, _iptr);
		str[_recordlen] = 0;
		_recordlen = 0;
	}
	return str;
}

void GDSParse::ReportUnsupported(const char *Name, enum RecordNumbers rn)
{
	if(!_unsupported[rn]){
		v_printf(2, "Unsupported GDS2 record type: %s\n", Name);
		_unsupported[rn] = true;
	}

}

class GDSProcess *GDSParse::GetProcess() {
  if(_process != NULL){
		return _process;
	}
  return NULL;
}
