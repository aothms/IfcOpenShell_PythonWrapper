/********************************************************************************
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

#ifndef OPENCASCADEBASEDSERIALIZER_H
#define OPENCASCADEBASEDSERIALIZER_H

#include "../ifcgeom/IfcGeomObjects.h"

#include "../ifcconvert/GeometrySerializer.h"

class OpenCascadeBasedSerializer : public GeometrySerializer {
protected:
	const std::string& out_filename;
	const char* getSymbolForUnitMagnitude(float mag);
public:
	explicit OpenCascadeBasedSerializer(const std::string& out_filename)
		: GeometrySerializer()
		, out_filename(out_filename)
	{}
	virtual ~OpenCascadeBasedSerializer() {}
	void writeHeader() {}
	void writeMaterial(const IfcGeom::SurfaceStyle& style) {}
	bool ready();
	virtual void writeShape(const TopoDS_Shape& shape) = 0;
	void writeTesselated(const IfcGeomObjects::IfcGeomObject* o) {}
	void writeShapeModel(const IfcGeomObjects::IfcGeomShapeModelObject* o);
	bool isTesselated() const { return false; }
};

#endif