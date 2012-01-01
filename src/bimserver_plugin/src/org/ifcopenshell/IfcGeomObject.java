/*******************************************************************************
*                                                                              *
* This file is part of IfcOpenShell.                                           *
*                                                                              *
* IfcOpenShell is free software: you can redistribute it and/or modify         *
* it under the terms of the Lesser GNU General Public License as published by  *
* the Free Software Foundation, either version 3.0 of the License, or          *
* (at your option) any later version.                                          *
*                                                                              *
* IfcOpenShell is distributed in the hope that it will be useful,              *
* but WITHOUT ANY WARRANTY; without even the implied warranty of               *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
* Lesser GNU General Public License for more details.                          *
*                                                                              *
* You should have received a copy of the Lesser GNU General Public License     *
* along with this program. If not, see <http://www.gnu.org/licenses/>.         *
*                                                                              *
********************************************************************************/

/*******************************************************************************
*                                                                              *
* This class is a literal translation of the interface class IfcGeomObject in  *
* the C++ implementation of IfcOpenShell, these babies are returned from the   *
* JNI interface.                                                               *
*                                                                              *
********************************************************************************/

package org.ifcopenshell;

public class IfcGeomObject {
	public int id;
	public String name;
	public String type;
	public String guid;
	public int[] indices;
	public float[] vertices;
	public float[] normals;
	
	public IfcGeomObject(int i, String nm, String t, String g, int[] is, float[] vs, float[] ns) {
		id = i;
		name = nm;
		type = t;
		guid = g;
		indices = is;
		vertices = vs;
		normals = ns;
	}
}
