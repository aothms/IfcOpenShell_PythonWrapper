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

#include <string>
#include <fstream>
#include <cstdio>

#include <BRepBuilderAPI_GTransform.hxx>
#include <BRepBuilderAPI_Transform.hxx>

#include <Standard_Version.hxx>

#include "OpenCascadeBasedSerializer.h"

bool OpenCascadeBasedSerializer::ready() {
	std::ofstream test_file(out_filename.c_str(), std::ios_base::binary);
	bool succeeded = test_file.is_open();
	test_file.close();
	remove(out_filename.c_str());
	return succeeded;
}

void OpenCascadeBasedSerializer::write(const IfcGeom::BRepElement<double>* o) {		
	for (IfcGeom::IfcRepresentationShapeItems::const_iterator it = o->geometry().begin(); it != o->geometry().end(); ++ it) {
		gp_GTrsf gtrsf = it->Placement();

		const std::vector<double>& matrix = o->transformation().matrix().data();
		
		// Convert the matrix back into a transformation object. The tolerance values
		// are taken into consideration to reconstruct the form of the transformation.
		gp_Trsf o_trsf;
		o_trsf.SetValues(
			matrix[0], matrix[3], matrix[6], matrix[ 9],
			matrix[1], matrix[4], matrix[7], matrix[10], 
			matrix[2], matrix[5], matrix[8], matrix[11]
#if OCC_VERSION_HEX < 0x60800
			, Precision::Angular(), Precision::Confusion()
#endif
		);
		gtrsf.PreMultiply(o_trsf);
		
		const TopoDS_Shape& s = it->Shape();			
			
		bool trsf_valid = false;
		gp_Trsf trsf;
		try {
			trsf = gtrsf.Trsf();
			trsf_valid = true;
		} catch (...) {}
			
		const TopoDS_Shape moved_shape = trsf_valid
			? BRepBuilderAPI_Transform(s, trsf, true).Shape()
			: BRepBuilderAPI_GTransform(s, gtrsf, true).Shape();
			
		writeShape(moved_shape);
	}
}

#define RATHER_SMALL (1e-3)
#define APPROXIMATELY_THE_SAME(a,b) (fabs(a-b) < RATHER_SMALL)

const char* OpenCascadeBasedSerializer::getSymbolForUnitMagnitude(float mag) {
	if (APPROXIMATELY_THE_SAME(mag, 0.001f)) {
		return "MM";
	} else if (APPROXIMATELY_THE_SAME(mag, 0.01f)) {
		return "CM";
	} else if (APPROXIMATELY_THE_SAME(mag, 1.0f)) {
		return "M";
	} else if (APPROXIMATELY_THE_SAME(mag, 0.3048f)) {
		return "FT";
	} else if (APPROXIMATELY_THE_SAME(mag, 0.0254f)) {
		return "INCH";
	} else {
		return 0;
	}
}
