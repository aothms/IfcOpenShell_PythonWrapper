﻿/********************************************************************************
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

/********************************************************************************
 *                                                                              *
 * Implementations of the various conversion functions defined in IfcRegister.h *
 *                                                                              *
 ********************************************************************************/

#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Vec2d.hxx>
#include <gp_Dir2d.hxx>
#include <gp_Mat.hxx>
#include <gp_Mat2d.hxx>
#include <gp_GTrsf.hxx>
#include <gp_GTrsf2d.hxx>
#include <gp_Trsf.hxx>
#include <gp_Trsf2d.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax3.hxx>
#include <gp_Ax2d.hxx>
#include <gp_Pln.hxx>
#include <gp_Circ.hxx>

#include <TColgp_Array1OfPnt.hxx>
#include <TColgp_Array1OfPnt2d.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TColStd_Array1OfInteger.hxx>

#include <Geom_Line.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Ellipse.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Geom_CylindricalSurface.hxx>

#include <BRepOffsetAPI_Sewing.hxx>
#include <BRepOffsetAPI_MakePipe.hxx>
#include <BRepOffsetAPI_MakePipeShell.hxx>

#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>

#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Face.hxx>
#include <TopExp_Explorer.hxx>

#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeWedge.hxx>

#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBuilderAPI_MakeShell.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepPrimAPI_MakeHalfSpace.hxx>

#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Common.hxx>

#include <ShapeFix_Shape.hxx>
#include <ShapeFix_ShapeTolerance.hxx>
#include <ShapeFix_Solid.hxx>

#include <TopLoc_Location.hxx>

#include <BRepCheck_Analyzer.hxx>

#include "../ifcgeom/IfcGeom.h"

bool IfcGeom::convert(const IfcSchema::IfcExtrudedAreaSolid::ptr l, TopoDS_Shape& shape) {
	TopoDS_Face face;
	if ( ! IfcGeom::convert_face(l->SweptArea(),face) ) return false;
	const double height = l->Depth() * IfcGeom::GetValue(GV_LENGTH_UNIT);
	gp_Trsf trsf;
	IfcGeom::convert(l->Position(),trsf);

	gp_Dir dir;
	convert(l->ExtrudedDirection(),dir);

	shape = BRepPrimAPI_MakePrism(face,height*dir);
	shape.Move(trsf);
	return ! shape.IsNull();
}

bool IfcGeom::convert(const IfcSchema::IfcSurfaceOfLinearExtrusion::ptr l, TopoDS_Shape& shape) {
	TopoDS_Wire wire;
	if ( !IfcGeom::convert_wire(l->SweptCurve(), wire) ) {
		TopoDS_Face face;
		if ( !IfcGeom::convert_face(l->SweptCurve(),face) ) return false;
		TopExp_Explorer exp(face, TopAbs_WIRE);
		wire = TopoDS::Wire(exp.Current());
	}
	const double height = l->Depth() * IfcGeom::GetValue(GV_LENGTH_UNIT);
	gp_Trsf trsf;
	IfcGeom::convert(l->Position(),trsf);

	gp_Dir dir;
	convert(l->ExtrudedDirection(),dir);

	shape = BRepPrimAPI_MakePrism(wire, height*dir);
	shape.Move(trsf);
	return !shape.IsNull();
}

bool IfcGeom::convert(const IfcSchema::IfcSurfaceOfRevolution::ptr l, TopoDS_Shape& shape) {
	TopoDS_Wire wire;
	if ( !IfcGeom::convert_wire(l->SweptCurve(), wire) ) {
		TopoDS_Face face;
		if ( !IfcGeom::convert_face(l->SweptCurve(),face) ) return false;
		TopExp_Explorer exp(face, TopAbs_WIRE);
		wire = TopoDS::Wire(exp.Current());
	}

	gp_Ax1 ax1;
	IfcGeom::convert(l->AxisPosition(), ax1);

	gp_Trsf trsf;
	IfcGeom::convert(l->Position(),trsf);
	
	shape = BRepPrimAPI_MakeRevol(wire, ax1);

	shape.Move(trsf);
	return !shape.IsNull();
}

bool IfcGeom::convert(const IfcSchema::IfcRevolvedAreaSolid::ptr l, TopoDS_Shape& shape) {
	const double ang = l->Angle() * IfcGeom::GetValue(GV_PLANEANGLE_UNIT);

	TopoDS_Face face;
	if ( ! IfcGeom::convert_face(l->SweptArea(),face) ) return false;

	gp_Ax1 ax1;
	IfcGeom::convert(l->Axis(), ax1);

	gp_Trsf trsf;
	IfcGeom::convert(l->Position(),trsf);

	if (ang >= M_PI * 2. - ALMOST_ZERO) {
		shape = BRepPrimAPI_MakeRevol(face, ax1);
	} else {
		shape = BRepPrimAPI_MakeRevol(face, ax1, ang);
	}

	shape.Move(trsf);
	return !shape.IsNull();
}

bool IfcGeom::convert(const IfcSchema::IfcFacetedBrep::ptr l, IfcRepresentationShapeItems& shape) {
	TopoDS_Shape s;
	if (IfcGeom::convert_shape(l->Outer(),s) ) {
		shape.push_back(IfcRepresentationShapeItem(s, get_style(l->Outer())));
		return true;
	}
	return false;
}

bool IfcGeom::convert(const IfcSchema::IfcFaceBasedSurfaceModel::ptr l, IfcRepresentationShapeItems& shapes) {
	IfcSchema::IfcConnectedFaceSet::list facesets = l->FbsmFaces();
	const SurfaceStyle* collective_style = get_style(l);
	for( IfcSchema::IfcConnectedFaceSet::it it = facesets->begin(); it != facesets->end(); ++ it ) {
		TopoDS_Shape s;
		const SurfaceStyle* shell_style = get_style(*it);
		if (IfcGeom::convert_shape(*it,s)) {
			shapes.push_back(IfcRepresentationShapeItem(s, shell_style ? shell_style : collective_style));
		}
	}
	return true;
}

bool IfcGeom::convert(const IfcSchema::IfcHalfSpaceSolid::ptr l, TopoDS_Shape& shape) {
	IfcSchema::IfcSurface::ptr surface = l->BaseSurface();
	if ( ! surface->is(IfcSchema::Type::IfcPlane) ) {
		Logger::Message(Logger::LOG_ERROR, "Unsupported BaseSurface:", surface->entity);
		return false;
	}
	gp_Pln pln;
	IfcGeom::convert(reinterpret_pointer_cast<IfcSchema::IfcSurface,IfcSchema::IfcPlane>(surface),pln);
	const gp_Pnt pnt = pln.Location().Translated( l->AgreementFlag() ? -pln.Axis().Direction() : pln.Axis().Direction());
	shape = BRepPrimAPI_MakeHalfSpace(BRepBuilderAPI_MakeFace(pln),pnt).Solid();
	return true;
}

bool IfcGeom::convert(const IfcSchema::IfcPolygonalBoundedHalfSpace::ptr l, TopoDS_Shape& shape) {
	TopoDS_Shape halfspace;
	if ( ! IfcGeom::convert(reinterpret_pointer_cast<IfcSchema::IfcPolygonalBoundedHalfSpace,IfcSchema::IfcHalfSpaceSolid>(l),halfspace) ) return false;	
	TopoDS_Wire wire;
	if ( ! IfcGeom::convert_wire(l->PolygonalBoundary(),wire) || ! wire.Closed() ) return false;	
	gp_Trsf trsf;
	convert(l->Position(),trsf);
	TopoDS_Shape prism = BRepPrimAPI_MakePrism(BRepBuilderAPI_MakeFace(wire),gp_Vec(0,0,200));
	gp_Trsf down; down.SetTranslation(gp_Vec(0,0,-100.0));
	prism.Move(trsf*down);
	shape = BRepAlgoAPI_Common(halfspace,prism);
	return true;
}

bool IfcGeom::convert(const IfcSchema::IfcShellBasedSurfaceModel::ptr l, IfcRepresentationShapeItems& shapes) {
	IfcUtil::IfcAbstractSelect::list shells = l->SbsmBoundary();
	const SurfaceStyle* collective_style = get_style(l);
	for( IfcUtil::IfcAbstractSelect::it it = shells->begin(); it != shells->end(); ++ it ) {
		TopoDS_Shape s;
		const SurfaceStyle* shell_style = 0;
		if ((*it)->is(IfcSchema::Type::IfcRepresentationItem)) {
			shell_style = get_style((IfcSchema::IfcRepresentationItem*)*it);
		}
		if (IfcGeom::convert_shape(*it,s)) {
			shapes.push_back(IfcRepresentationShapeItem(s, shell_style ? shell_style : collective_style));
		}
	}
	return true;
}

bool IfcGeom::convert(const IfcSchema::IfcBooleanResult::ptr l, TopoDS_Shape& shape) {
	TopoDS_Shape s1, s2;
	TopoDS_Wire boundary_wire;
	IfcSchema::IfcBooleanOperand operand1 = l->FirstOperand();
	IfcSchema::IfcBooleanOperand operand2 = l->SecondOperand();
	bool is_halfspace = operand2->is(IfcSchema::Type::IfcHalfSpaceSolid);

	if ( ! IfcGeom::convert_shape(operand1,s1) )
		return false;

	const double first_operand_volume = shape_volume(s1);
	if ( first_operand_volume <= ALMOST_ZERO )
		Logger::Message(Logger::LOG_WARNING,"Empty solid for:",l->FirstOperand()->entity);

	if ( !IfcGeom::convert_shape(l->SecondOperand(),s2) ) {
		shape = s1;
		Logger::Message(Logger::LOG_ERROR,"Failed to convert SecondOperand of:",l->entity);
		return true;
	}

	if ( ! is_halfspace ) {
		const double second_operand_volume = shape_volume(s2);
		if ( second_operand_volume <= ALMOST_ZERO )
			Logger::Message(Logger::LOG_WARNING,"Empty solid for:",operand2->entity);
	}

	const IfcSchema::IfcBooleanOperator::IfcBooleanOperator op = l->Operator();

	if (op == IfcSchema::IfcBooleanOperator::IfcBooleanOperator_DIFFERENCE) {

		bool valid_cut = false;
		BRepAlgoAPI_Cut brep_cut(s1,s2);
		if ( brep_cut.IsDone() ) {
			TopoDS_Shape result = brep_cut;

			ShapeFix_Shape fix(result);
			fix.Perform();
			result = fix.Shape();
		
			bool is_valid = BRepCheck_Analyzer(result).IsValid() != 0;
			if ( is_valid ) {
				shape = result;
				valid_cut = true;
			} 
		}

		if ( valid_cut ) {
			const double volume_after_subtraction = shape_volume(shape);
			if ( ALMOST_THE_SAME(first_operand_volume,volume_after_subtraction) )
				Logger::Message(Logger::LOG_WARNING,"Subtraction yields unchanged volume:",l->entity);
		} else {
			Logger::Message(Logger::LOG_ERROR,"Failed to process subtraction:",l->entity);
			shape = s1;
		}

		return true;

	} else if (op == IfcSchema::IfcBooleanOperator::IfcBooleanOperator_UNION) {

		BRepAlgoAPI_Fuse brep_fuse(s1,s2);
		if ( brep_fuse.IsDone() ) {
			TopoDS_Shape result = brep_fuse;

			ShapeFix_Shape fix(result);
			fix.Perform();
			result = fix.Shape();
		
			bool is_valid = BRepCheck_Analyzer(result).IsValid() != 0;
			if ( is_valid ) {
				shape = result;
			} 
		}

		return true;

	} else if (op == IfcSchema::IfcBooleanOperator::IfcBooleanOperator_INTERSECTION) {

		BRepAlgoAPI_Common brep_common(s1,s2);
		if ( brep_common.IsDone() ) {
			TopoDS_Shape result = brep_common;

			ShapeFix_Shape fix(result);
			fix.Perform();
			result = fix.Shape();
		
			bool is_valid = BRepCheck_Analyzer(result).IsValid() != 0;
			if ( is_valid ) {
				shape = result;
			} 
		}

		return true;

	} else {
		return false;
	}
}

bool IfcGeom::convert(const IfcSchema::IfcConnectedFaceSet::ptr l, TopoDS_Shape& shape) {
	IfcSchema::IfcFace::list faces = l->CfsFaces();
	bool facesAdded = false;
	const unsigned int num_faces = faces->Size();
	if ( num_faces < GetValue(GV_MAX_FACES_TO_SEW) ) {
		BRepOffsetAPI_Sewing builder;
		builder.SetTolerance(GetValue(GV_POINT_EQUALITY_TOLERANCE));
		builder.SetMaxTolerance(GetValue(GV_POINT_EQUALITY_TOLERANCE));
		builder.SetMinTolerance(GetValue(GV_POINT_EQUALITY_TOLERANCE));
		for( IfcSchema::IfcFace::it it = faces->begin(); it != faces->end(); ++ it ) {
			TopoDS_Face face;
			if ( IfcGeom::convert_face(*it,face) && face_area(face) > GetValue(GV_MINIMAL_FACE_AREA) ) {
				builder.Add(face);
				facesAdded = true;
			} else {
				Logger::Message(Logger::LOG_WARNING,"Invalid face:",(*it)->entity);
			}
		}
		if ( ! facesAdded ) return false;
		builder.Perform();
		shape = builder.SewedShape();
		try {
			ShapeFix_Solid solid;
			solid.LimitTolerance(GetValue(GV_POINT_EQUALITY_TOLERANCE));
			shape = solid.SolidFromShell(TopoDS::Shell(shape));
		} catch(...) {}
	} else {
		TopoDS_Compound compound;
		BRep_Builder builder;
		builder.MakeCompound(compound);
		for( IfcSchema::IfcFace::it it = faces->begin(); it != faces->end(); ++ it ) {
			TopoDS_Face face;
			if ( IfcGeom::convert_face(*it,face) && face_area(face) > GetValue(GV_MINIMAL_FACE_AREA) ) {
				builder.Add(compound,face);
				facesAdded = true;
			} else {
				Logger::Message(Logger::LOG_WARNING,"Invalid face:",(*it)->entity);
			}
		}
		if ( ! facesAdded ) return false;
		shape = compound;
	}
	return true;
}

bool IfcGeom::convert(const IfcSchema::IfcMappedItem::ptr l, IfcRepresentationShapeItems& shapes) {
	gp_GTrsf gtrsf;
	IfcSchema::IfcCartesianTransformationOperator::ptr transform = l->MappingTarget();
	if ( transform->is(IfcSchema::Type::IfcCartesianTransformationOperator3DnonUniform) ) {
		IfcGeom::convert(reinterpret_pointer_cast<IfcSchema::IfcCartesianTransformationOperator,
			IfcSchema::IfcCartesianTransformationOperator3DnonUniform>(transform),gtrsf);
	} else if ( transform->is(IfcSchema::Type::IfcCartesianTransformationOperator2DnonUniform) ) {
		Logger::Message(Logger::LOG_ERROR, "Unsupported MappingTarget:", transform->entity);
		return false;
	} else if ( transform->is(IfcSchema::Type::IfcCartesianTransformationOperator3D) ) {
		gp_Trsf trsf;
		IfcGeom::convert(reinterpret_pointer_cast<IfcSchema::IfcCartesianTransformationOperator,
			IfcSchema::IfcCartesianTransformationOperator3D>(transform),trsf);
		gtrsf = trsf;
	} else if ( transform->is(IfcSchema::Type::IfcCartesianTransformationOperator2D) ) {
		gp_Trsf2d trsf_2d;
		IfcGeom::convert(reinterpret_pointer_cast<IfcSchema::IfcCartesianTransformationOperator,
			IfcSchema::IfcCartesianTransformationOperator2D>(transform),trsf_2d);
		gtrsf = (gp_Trsf) trsf_2d;
	}
	IfcSchema::IfcRepresentationMap::ptr map = l->MappingSource();
	IfcSchema::IfcAxis2Placement placement = map->MappingOrigin();
	gp_Trsf trsf;
	if (placement->is(IfcSchema::Type::IfcAxis2Placement3D)) {
		IfcGeom::convert((IfcSchema::IfcAxis2Placement3D*)placement,trsf);
	} else {
		gp_Trsf2d trsf_2d;
		IfcGeom::convert((IfcSchema::IfcAxis2Placement2D*)placement,trsf_2d);
		trsf = trsf_2d;
	}
	gtrsf.Multiply(trsf);
	const unsigned int previous_size = (const unsigned int) shapes.size();
	bool b = IfcGeom::convert_shapes(map->MappedRepresentation(),shapes);
	for ( unsigned int i = previous_size; i < shapes.size(); ++ i ) {
		shapes[i].append(gtrsf);
	}
	return b;
}

bool IfcGeom::convert(const IfcSchema::IfcShapeRepresentation::ptr l, IfcRepresentationShapeItems& shapes) {
	IfcSchema::IfcRepresentationItem::list items = l->Items();
	if ( ! items->Size() ) return false;
	for ( IfcSchema::IfcRepresentationItem::it it = items->begin(); it != items->end(); ++ it ) {
		IfcSchema::IfcRepresentationItem* representation_item = *it;
		if ( IfcGeom::is_shape_collection(representation_item) ) IfcGeom::convert_shapes(*it,shapes);
		else {
			TopoDS_Shape s;
			if (IfcGeom::convert_shape(representation_item,s)) {
				shapes.push_back(IfcRepresentationShapeItem(s, get_style(representation_item)));
			}
		}
	}
	return true;
}

bool IfcGeom::convert(const IfcSchema::IfcGeometricSet::ptr l, IfcRepresentationShapeItems& shapes) {
	IfcUtil::IfcAbstractSelect::list elements = l->Elements();
	if ( !elements->Size() ) return false;
	const IfcGeom::SurfaceStyle* parent_style = get_style(l);
	for ( IfcUtil::IfcAbstractSelect::it it = elements->begin(); it != elements->end(); ++ it ) {
		IfcSchema::IfcGeometricSetSelect element = *it;
		if (element->is(IfcSchema::Type::IfcSurface)) {
			IfcSchema::IfcSurface* surface = (IfcSchema::IfcSurface*) element;
			TopoDS_Shape s;
			if (IfcGeom::convert_shape(surface, s)) {
				const IfcGeom::SurfaceStyle* style = get_style(surface);
				shapes.push_back(IfcRepresentationShapeItem(s, style ? style : parent_style));
			}
		}
	}
	return true;
}

bool IfcGeom::convert(const IfcSchema::IfcBlock::ptr l, TopoDS_Shape& shape) {
	const double dx = l->XLength() * IfcGeom::GetValue(GV_LENGTH_UNIT);
	const double dy = l->YLength() * IfcGeom::GetValue(GV_LENGTH_UNIT);
	const double dz = l->ZLength() * IfcGeom::GetValue(GV_LENGTH_UNIT);

	BRepPrimAPI_MakeBox builder(dx, dy, dz);
	gp_Trsf trsf;
	IfcGeom::convert(l->Position(),trsf);
	shape = builder.Solid().Moved(trsf);
	return true;
}

bool IfcGeom::convert(const IfcSchema::IfcRectangularPyramid::ptr l, TopoDS_Shape& shape) {
	const double dx = l->XLength() * IfcGeom::GetValue(GV_LENGTH_UNIT);
	const double dy = l->YLength() * IfcGeom::GetValue(GV_LENGTH_UNIT);
	const double dz = l->Height() * IfcGeom::GetValue(GV_LENGTH_UNIT);
	
	BRepPrimAPI_MakeWedge builder(dx, dz, dy, dx / 2., dy / 2., dx / 2., dy / 2.);
	
	gp_Trsf trsf1, trsf2;
	trsf2.SetValues(1, 0, 0, 0,
					0, 0, 1, 0,
					0, 1, 0, 0, Precision::Confusion(), Precision::Confusion());
	
	IfcGeom::convert(l->Position(), trsf1);
	shape = BRepBuilderAPI_Transform(builder.Solid(), trsf1 * trsf2);
	return true;
}

bool IfcGeom::convert(const IfcSchema::IfcRightCircularCylinder::ptr l, TopoDS_Shape& shape) {
	const double r = l->Radius() * IfcGeom::GetValue(GV_LENGTH_UNIT);
	const double h = l->Height() * IfcGeom::GetValue(GV_LENGTH_UNIT);

	BRepPrimAPI_MakeCylinder builder(r, h);
	gp_Trsf trsf;
	IfcGeom::convert(l->Position(),trsf);
	shape = builder.Solid().Moved(trsf);
	return true;
}

bool IfcGeom::convert(const IfcSchema::IfcRightCircularCone::ptr l, TopoDS_Shape& shape) {
	const double r = l->BottomRadius() * IfcGeom::GetValue(GV_LENGTH_UNIT);
	const double h = l->Height() * IfcGeom::GetValue(GV_LENGTH_UNIT);

	BRepPrimAPI_MakeCone builder(r, 0., h);
	gp_Trsf trsf;
	IfcGeom::convert(l->Position(),trsf);
	shape = builder.Solid().Moved(trsf);
	return true;
}

bool IfcGeom::convert(const IfcSchema::IfcSphere::ptr l, TopoDS_Shape& shape) {
	const double r = l->Radius() * IfcGeom::GetValue(GV_LENGTH_UNIT);
	
	BRepPrimAPI_MakeSphere builder(r);
	gp_Trsf trsf;
	IfcGeom::convert(l->Position(),trsf);
	shape = builder.Solid().Moved(trsf);
	return true;
}

bool IfcGeom::convert(const IfcSchema::IfcCsgSolid::ptr l, TopoDS_Shape& shape) {
	return IfcGeom::convert_shape(l->TreeRootExpression(), shape);
}

bool IfcGeom::convert(const IfcSchema::IfcCurveBoundedPlane::ptr l, TopoDS_Shape& face) {
	gp_Pln pln;
	IfcGeom::convert(l->BasisSurface(), pln);

	gp_Trsf trsf;
	trsf.SetTransformation(pln.Position());
	
	TopoDS_Wire outer;
	IfcGeom::convert_wire(l->OuterBoundary(), outer);
	
	BRepBuilderAPI_MakeFace mf (outer);
	mf.Add(outer);

	IfcSchema::IfcCurve::list inner = l->InnerBoundaries();

	for (IfcSchema::IfcCurve::it it = inner->begin(); it != inner->end(); ++it) {
		TopoDS_Wire inner;
		IfcGeom::convert_wire(*it, inner);
		
		mf.Add(inner);
	}

	ShapeFix_Shape sfs(mf.Face());
	sfs.Perform();
	face = TopoDS::Face(sfs.Shape()).Moved(trsf);
	
	return true;
}

bool IfcGeom::convert(const IfcSchema::IfcRectangularTrimmedSurface::ptr l, TopoDS_Shape& face) {
	if (!l->BasisSurface()->is(IfcSchema::Type::IfcPlane)) {
		Logger::Message(Logger::LOG_ERROR, "Unsupported BasisSurface:", l->BasisSurface()->entity);
		return false;
	}
	gp_Pln pln;
	IfcGeom::convert((IfcSchema::IfcPlane*) l->BasisSurface(), pln);

	BRepBuilderAPI_MakeFace mf(pln, l->U1(), l->U2(), l->V1(), l->V2());

	face = mf.Face();

	return true;
}

bool IfcGeom::convert(const IfcSchema::IfcSurfaceCurveSweptAreaSolid::ptr l, TopoDS_Shape& shape) {
	gp_Trsf directrix, position;
	TopoDS_Shape face;
	TopoDS_Wire wire, section;

	if (!l->ReferenceSurface()->is(IfcSchema::Type::IfcPlane)) {
		Logger::Message(Logger::LOG_WARNING, "Reference surface not supported", l->ReferenceSurface()->entity);
		return false;
	}
	
	if (!IfcGeom::convert(l->Position(), position)   || 
		!IfcGeom::convert_face(l->SweptArea(), face) || 
		!IfcGeom::convert_wire(l->Directrix(), wire)    ) {
		return false;
	}

	gp_Pln pln;
	gp_Pnt directrix_origin;
	gp_Vec directrix_tangent;
	bool directrix_on_plane = true;
	IfcGeom::convert((IfcSchema::IfcPlane*) l->ReferenceSurface(), pln);

	// As per Informal propositions 2: The Directrix shall lie on the ReferenceSurface.
	// This is not always the case with the test files in the repository. I am not sure
	// how to deal with this and whether my interpretation of the propositions is
	// correct. However, if it has been asserted that the vertices of the directrix do
	// not conform to the ReferenceSurface, the ReferenceSurface is ignored.
	{ 
		for (TopExp_Explorer exp(wire, TopAbs_VERTEX); exp.More(); exp.Next()) {
			if (pln.Distance(BRep_Tool::Pnt(TopoDS::Vertex(exp.Current()))) > ALMOST_ZERO) {
				directrix_on_plane = false;
				Logger::Message(Logger::LOG_WARNING, "The Directrix does not lie on the ReferenceSurface", l->entity);
				break;
			}
		}
	}

	{ 
		TopExp_Explorer exp(wire, TopAbs_EDGE);
		TopoDS_Edge edge = TopoDS::Edge(exp.Current());
		double u0, u1;
		Handle(Geom_Curve) crv = BRep_Tool::Curve(edge, u0, u1);
		crv->D1(u0, directrix_origin, directrix_tangent);
	}

	if (pln.Axis().Direction().IsNormal(directrix_tangent, Precision::Approximation()) && directrix_on_plane) {
		directrix.SetTransformation(gp_Ax3(directrix_origin, directrix_tangent, pln.Axis().Direction()), gp::XOY());
	} else {
		directrix.SetTransformation(gp_Ax3(directrix_origin, directrix_tangent), gp::XOY());
	}
	face = BRepBuilderAPI_Transform(face, directrix);

	// NB: Note that StartParam and EndParam param are ignored and the assumption is
	// made that the parametric range over which to be swept matches the IfcCurve in
	// its entirety.
	BRepOffsetAPI_MakePipeShell builder(wire);

	{ TopExp_Explorer exp(face, TopAbs_WIRE);
	section = TopoDS::Wire(exp.Current()); }

	builder.Add(section);
	builder.SetTransitionMode(BRepBuilderAPI_RightCorner);
	if (directrix_on_plane) {
		builder.SetMode(pln.Axis().Direction());
	}
	builder.Build();
	builder.MakeSolid();
	shape = builder.Shape();
	shape.Move(position);

	return true;
}

#ifdef USE_IFC4

bool IfcGeom::convert(const IfcSchema::IfcCylindricalSurface::ptr l, TopoDS_Shape& face) {
	gp_Trsf trsf;
	IfcGeom::convert(l->Position(),trsf);
	
	face = BRepBuilderAPI_MakeFace(new Geom_CylindricalSurface(gp::XOY(), l->Radius()), GetValue(GV_PRECISION)).Face().Moved(trsf);
	return true;
}

#endif