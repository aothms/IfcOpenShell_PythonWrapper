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

#ifndef GEOMETRYSERIALIZER_H
#define GEOMETRYSERIALIZER_H

#include "../ifcconvert/Serializer.h"
#include "../ifcgeom/IfcGeomIterator.h"

class GeometrySerializer : public Serializer {
public:
	virtual ~GeometrySerializer() {} 

	virtual bool isTesselated() const = 0;
	virtual void write(const IfcGeom::TriangulationElement<double>* o) = 0;
	virtual void write(const IfcGeom::BRepElement<double>* o) = 0;
	virtual void setUnitNameAndMagnitude(const std::string& name, float magnitude) = 0;
};

#endif