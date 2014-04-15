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

#include <map>
#include <stdexcept>
#include <limits>

#include <gp_Mat.hxx>
#include <gp_Mat2d.hxx>
#include <gp_GTrsf.hxx>
#include <gp_GTrsf2d.hxx>
#include <gp_Trsf.hxx>
#include <gp_Trsf2d.hxx>
#include <TopoDS_Compound.hxx>
#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <TopExp_Explorer.hxx>
#include <BRepMesh.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_PolygonOnTriangulation.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TColgp_Array1OfPnt2d.hxx>
#include <TShort_Array1OfShortReal.hxx>
#include <Poly_Array1OfTriangle.hxx>
#include <StdFail_NotDone.hxx>
#include <BRepGProp_Face.hxx>
#include <BRepBuilderAPI_GTransform.hxx>

#include "../ifcparse/IfcException.h"
#include "../ifcparse/IfcFile.h"
#include "../ifcgeom/IfcGeomObjects.h"
#include "../ifcgeom/IfcGeom.h"

// Welds vertices that belong to different faces
static bool weld_vertices = true;

static bool convert_back_units = false;
static bool use_faster_booleans = false;
static bool disable_subtractions = false;
static bool disable_triangulation = false;

int IfcGeomObjects::IfcRepresentationTriangulation::addvert(int material_index, const gp_XYZ& p) {
	const float X = convert_back_units ? (float) (p.X() / IfcGeom::GetValue(IfcGeom::GV_LENGTH_UNIT)) : (float)p.X();
	const float Y = convert_back_units ? (float) (p.Y() / IfcGeom::GetValue(IfcGeom::GV_LENGTH_UNIT)) : (float)p.Y();
	const float Z = convert_back_units ? (float) (p.Z() / IfcGeom::GetValue(IfcGeom::GV_LENGTH_UNIT)) : (float)p.Z();
	int i = (int) _verts.size() / 3;
	if ( weld_vertices ) {
		const VertKey key = std::make_pair(material_index, std::make_pair(X, std::make_pair(Y, Z)));
		VertKeyMap::const_iterator it = welds.find(key);
		if ( it != welds.end() ) return it->second;
		i = (int) welds.size();
		welds[key] = i;
	}
	_verts.push_back(X);
	_verts.push_back(Y);
	_verts.push_back(Z);
	return i;
}

static bool use_world_coords = false;
static bool use_brep_data = false;

static IfcParse::IfcFile* ifc_file = 0;

IfcGeomObjects::IfcRepresentationBrepData::IfcRepresentationBrepData(const IfcRepresentationShapeModel& shapes)
	: _id(shapes.getId()) 
{
	try {
		TopoDS_Compound compound;
		BRep_Builder builder;
		builder.MakeCompound(compound);
		for ( IfcGeom::IfcRepresentationShapeItems::const_iterator it = shapes.begin(); it != shapes.end(); ++ it ) {
			const TopoDS_Shape& s = it->Shape();
			gp_GTrsf trsf = it->Placement();
			if (convert_back_units) {
				gp_Trsf scale;
				scale.SetScaleFactor(1.0 / IfcGeom::GetValue(IfcGeom::GV_LENGTH_UNIT));
				trsf.PreMultiply(scale);
			}
			bool trsf_valid = false;
			gp_Trsf _trsf;
			try {
				_trsf = trsf.Trsf();
				trsf_valid = true;
			} catch (...) {}
			const TopoDS_Shape moved_shape = trsf_valid ? s.Moved(_trsf) :
				BRepBuilderAPI_GTransform(s,trsf,true).Shape();
			builder.Add(compound,moved_shape);
		}
		std::stringstream sstream;
		BRepTools::Write(compound,sstream);
		_brep_data = sstream.str();
	} catch(...) {
		Logger::Message(Logger::LOG_ERROR,"Failed to serialize shape:",ifc_file->EntityById(_id)->entity);
	}
}

IfcGeomObjects::IfcRepresentationTriangulation::IfcRepresentationTriangulation(const IfcRepresentationShapeModel& shapes)
	: _id(shapes.getId()) 
{
	for ( IfcGeom::IfcRepresentationShapeItems::const_iterator it = shapes.begin(); it != shapes.end(); ++ it ) {

		int surface_style_id = -1;
		if (it->hasStyle()) {
			Material adapter(&it->Style());
			std::vector<Material>::const_iterator jt = std::find(_materials.begin(), _materials.end(), adapter);
			if (jt == _materials.end()) {
				surface_style_id = _materials.size();
				_materials.push_back(adapter);
			} else {
				surface_style_id = jt - _materials.begin();
			}
		}

		const TopoDS_Shape& s = it->Shape();
		const gp_GTrsf& trsf = it->Placement();

		// Triangulate the shape
		try {
			// BRepTools::Clean(s);
			BRepMesh::Mesh(s, IfcGeom::GetValue(IfcGeom::GV_DEFLECTION_TOLERANCE));
		} catch(...) {
			Logger::Message(Logger::LOG_ERROR,"Failed to triangulate shape:",ifc_file->EntityById(_id)->entity);
			continue;
		}
		TopExp_Explorer exp;

		// Iterates over the faces of the shape
		for ( exp.Init(s,TopAbs_FACE); exp.More(); exp.Next() ) {
			TopoDS_Face face = TopoDS::Face(exp.Current());
			TopLoc_Location loc;
			Handle_Poly_Triangulation tri = BRep_Tool::Triangulation(face,loc);

			if ( ! tri.IsNull() ) {

				// A 3x3 matrix to rotate the vertex normals
				const gp_Mat rotation_matrix = trsf.VectorialPart();
			
				// Keep track of the number of times an edge is used
				// Manifold edges (i.e. edges used twice) are deemed invisible
				std::map<std::pair<int,int>,int> edgecount;
				std::vector<std::pair<int,int> > edges_temp;

				const TColgp_Array1OfPnt& nodes = tri->Nodes();
				const TColgp_Array1OfPnt2d& uvs = tri->UVNodes();
				std::vector<gp_XYZ> coords;
				BRepGProp_Face prop(face);
				std::map<int,int> dict;

				// Vertex normals are only calculated if vertices are not welded
				const bool calculate_normals = ! weld_vertices;

				for( int i = 1; i <= nodes.Length(); ++ i ) {
					coords.push_back(nodes(i).Transformed(loc).XYZ());
					trsf.Transforms(*coords.rbegin());
					dict[i] = addvert(surface_style_id, *coords.rbegin());
					
					if ( calculate_normals ) {
						const gp_Pnt2d& uv = uvs(i);
						gp_Pnt p;
						gp_Vec normal_direction;
						prop.Normal(uv.X(),uv.Y(),p,normal_direction);
						gp_Vec normal(0., 0., 0.);
						if (normal_direction.Magnitude() > ALMOST_ZERO) {
							normal = gp_Dir(normal_direction.XYZ() * rotation_matrix);
						}
						_normals.push_back((float)normal.X());
						_normals.push_back((float)normal.Y());
						_normals.push_back((float)normal.Z());
					}
				}

				const Poly_Array1OfTriangle& triangles = tri->Triangles();			
				for( int i = 1; i <= triangles.Length(); ++ i ) {
					int n1,n2,n3;
					if ( face.Orientation() == TopAbs_REVERSED )
						triangles(i).Get(n3,n2,n1);
					else triangles(i).Get(n1,n2,n3);

					/* An alternative would be to calculate normals based
					 * on the coordinates of the mesh vertices */
					/*
					const gp_XYZ pt1 = coords[n1-1];
					const gp_XYZ pt2 = coords[n2-1];
					const gp_XYZ pt3 = coords[n3-1];
					const gp_XYZ v1 = pt2-pt1;
					const gp_XYZ v2 = pt3-pt2;
					gp_Dir normal = gp_Dir(v1^v2);
					_normals.push_back((float)normal.X());
					_normals.push_back((float)normal.Y());
					_normals.push_back((float)normal.Z());
					*/

					_faces.push_back(dict[n1]);
					_faces.push_back(dict[n2]);
					_faces.push_back(dict[n3]);

					_material_ids.push_back(surface_style_id);

					addedge(n1,n2,edgecount,edges_temp);
					addedge(n2,n3,edgecount,edges_temp);
					addedge(n3,n1,edgecount,edges_temp);
				}
				for ( std::vector<std::pair<int,int> >::const_iterator it = edges_temp.begin(); it != edges_temp.end(); ++it ) {
					_edges.push_back(edgecount[*it]==1);
				}
			}
		}
	}
}

IfcGeomObjects::IfcObject::IfcObject(
	int id, 
	int parent_id, 
	const std::string& name, 
	const std::string& type, 
	const std::string& guid, 
	const gp_Trsf& trsf)
		: _id(id)
		, _parent_id(parent_id)
		, _name(name)
		, _type(type)
		, _guid(guid)
{
	// Convert the gp_Trsf into a 4x3 Matrix
	for( int i = 1; i < 5; ++ i )
		for ( int j = 1; j < 4; ++ j )
			_matrix.push_back((float)trsf.Value(j,i));
}

IfcGeomObjects::IfcGeomShapeModelObject::IfcGeomShapeModelObject(
	int id, 
	int parent_id, 
	const std::string& name, 
	const std::string& type, 
	const std::string& guid,
	const gp_Trsf& trsf, 
	IfcRepresentationShapeModel* shapes) 
		: IfcObject(id,parent_id,name,type,guid,trsf)
		, _mesh(shapes)
{}

IfcGeomObjects::IfcGeomBrepDataObject::IfcGeomBrepDataObject(
	const IfcGeomShapeModelObject& shape_model)
		: IfcObject(shape_model)
		, _mesh(new IfcRepresentationBrepData(shape_model.mesh()))
{}

IfcGeomObjects::IfcGeomObject::IfcGeomObject(
	const IfcGeomShapeModelObject& shape_model)
		: IfcObject(shape_model)
		, _mesh(new IfcRepresentationTriangulation(shape_model.mesh()))
{}

// A container and iterator for IfcShapeRepresentations
static IfcSchema::IfcShapeRepresentation::list shapereps;
static IfcSchema::IfcShapeRepresentation::it shaperep_iterator;

// The object is fetched beforehand to be positive an entity actually exists
static IfcGeomObjects::IfcGeomObject* current_geom_obj = 0;
static IfcGeomObjects::IfcGeomShapeModelObject* current_shape_model_obj = 0;
static IfcGeomObjects::IfcGeomBrepDataObject* current_brep_data_obj = 0;

// A container and iterator for IfcBuildingElements for the current IfcShapeRepresentation referenced by *shaperep_iterator
static IfcSchema::IfcProduct::list entities;
static IfcSchema::IfcProduct::it ifcproduct_iterator;

static int done;
static int total;

// Move the the next IfcShapeRepresentation
void _nextShape() {
	entities.reset();
	++ shaperep_iterator;
	++ done;
}

int _getParentId(const IfcSchema::IfcProduct::ptr ifc_product) {
	int parent_id = -1;
	// In case of an opening element, parent to the RelatingBuildingElement
	if ( ifc_product->is(IfcSchema::Type::IfcOpeningElement ) ) {
		IfcSchema::IfcOpeningElement::ptr opening = reinterpret_pointer_cast<IfcSchema::IfcProduct,IfcSchema::IfcOpeningElement>(ifc_product);
		IfcSchema::IfcRelVoidsElement::list voids = opening->VoidsElements();
		if ( voids->Size() ) {
			IfcSchema::IfcRelVoidsElement::ptr ifc_void = *voids->begin();
			parent_id = ifc_void->RelatingBuildingElement()->entity->id();
		}
	} else if ( ifc_product->is(IfcSchema::Type::IfcElement ) ) {
		IfcSchema::IfcElement::ptr element = reinterpret_pointer_cast<IfcSchema::IfcProduct,IfcSchema::IfcElement>(ifc_product);
		IfcSchema::IfcRelFillsElement::list fills = element->FillsVoids();
		// Incase of a RelatedBuildingElement parent to the opening element
		if ( fills->Size() ) {
			for ( IfcSchema::IfcRelFillsElement::it it = fills->begin(); it != fills->end(); ++ it ) {
				IfcSchema::IfcRelFillsElement::ptr fill = *it;
				IfcSchema::IfcObjectDefinition* ifc_objectdef = fill->RelatingOpeningElement();
				if ( ifc_product == ifc_objectdef ) continue;
				parent_id = ifc_objectdef->entity->id();
			}
		} 
		// Else simply parent to the containing structure
		if ( parent_id == -1 ) {
			IfcSchema::IfcRelContainedInSpatialStructure::list parents = element->ContainedInStructure();
			if ( parents->Size() ) {
				IfcSchema::IfcRelContainedInSpatialStructure::ptr parent = *parents->begin();
				parent_id = parent->RelatingStructure()->entity->id();
			}
		}
	}
	// Parent decompositions to the RelatingObject
	if ( parent_id == -1 ) {
		IfcEntities parents = ifc_product->entity->getInverse(IfcSchema::Type::IfcRelAggregates);
		parents->push(ifc_product->entity->getInverse(IfcSchema::Type::IfcRelNests));
		for ( IfcEntityList::it it = parents->begin(); it != parents->end(); ++ it ) {
			IfcSchema::IfcRelDecomposes::ptr decompose = reinterpret_pointer_cast<IfcUtil::IfcBaseClass,IfcSchema::IfcRelDecomposes>(*it);
			IfcSchema::IfcObjectDefinition* ifc_objectdef;
#ifdef USE_IFC4
			if (decompose->is(IfcSchema::Type::IfcRelAggregates)) {
				ifc_objectdef = ((IfcSchema::IfcRelAggregates*)decompose)->RelatingObject();
			} else {
				continue;
			}
#else
			ifc_objectdef = decompose->RelatingObject();
#endif
			if ( ifc_product == ifc_objectdef ) continue;
			parent_id = ifc_objectdef->entity->id();
		}
	}
	return parent_id;
}

IfcGeomObjects::IfcGeomShapeModelObject* create_shape_model_for_next_entity() {
	while ( true ) {
		IfcSchema::IfcShapeRepresentation::ptr shaperep;

		// Have we reached the end of our list of representations?
		if ( shaperep_iterator == shapereps->end() ) {
			shapereps.reset();
			return 0;
		}
		shaperep = *shaperep_iterator;

		// Has the list of IfcProducts for this representation been initialized?
		if ( ! entities ) {
			if ( shaperep->hasRepresentationIdentifier() ) {
				const std::string representation_identifier = shaperep->RepresentationIdentifier();
				if ( shaperep->hasRepresentationType() && representation_identifier == "IAI" && shaperep->RepresentationType() != "BoundingBox" ) {
					// Allow for Ifc 2x compatibility
				} else if ( representation_identifier != "Body" &&
					representation_identifier != "Facetation" ) {
						_nextShape();
						continue;
				}
			}
			IfcSchema::IfcProductRepresentation::list prodreps = shaperep->OfProductRepresentation();
			entities = IfcSchema::IfcProduct::list( new IfcTemplatedEntityList<IfcSchema::IfcProduct>() );
			for ( IfcSchema::IfcProductRepresentation::it it = prodreps->begin(); it != prodreps->end(); ++it ) {
				if ( (*it)->is(IfcSchema::Type::IfcProductDefinitionShape) ) {
					IfcSchema::IfcProductDefinitionShape::ptr pds = reinterpret_pointer_cast<IfcSchema::IfcProductRepresentation,IfcSchema::IfcProductDefinitionShape>(*it);
					entities->push(pds->ShapeOfProduct());
				} else {
					// http://buildingsmart-tech.org/ifc/IFC2x3/TC1/html/ifcrepresentationresource/lexical/ifcproductrepresentation.htm
					// IFC2x Edition 3 NOTE  Users should not instantiate the entity IfcProductRepresentation from IFC2x Edition 3 onwards. 
					// It will be changed into an ABSTRACT supertype in future releases of IFC.

					// IfcProductRepresentation also lacks the INVERSE relation to IfcProduct
					// Let's find the IfcProducts that reference the IfcProductRepresentation anyway
					IfcEntities products = (*it)->entity->getInverse(IfcSchema::Type::IfcProduct);
					for ( IfcEntityList::it it = products->begin(); it != products->end(); ++ it ) {
						entities->push(reinterpret_pointer_cast<IfcUtil::IfcBaseClass,IfcSchema::IfcProduct>(*it));
					}
				}
			}
			// Does this representation have any IfcProducts?
			if ( ! entities->Size() ) {
				_nextShape();
				continue;
			}
			ifcproduct_iterator = entities->begin();
		}
		// Have we reached the end of our list of IfcProducts?
		if ( ifcproduct_iterator == entities->end() ) {
			_nextShape();
			continue;
		}
		
		IfcGeomObjects::IfcRepresentationShapeModel* shape;
		IfcGeom::IfcRepresentationShapeItems shapes;

		if ( !IfcGeom::convert_shapes(shaperep,shapes) ) {
			_nextShape();
			continue;
		}

		IfcSchema::IfcProduct::ptr ifc_product = *ifcproduct_iterator;

		int parent_id = -1;
		try {
			parent_id = _getParentId(ifc_product);
		} catch (...) {}
		
		const std::string name = ifc_product->hasName() ? ifc_product->Name() : "";
		const std::string guid = ifc_product->GlobalId();
		
		gp_Trsf trsf;
		try {
			IfcGeom::convert(ifc_product->ObjectPlacement(),trsf);
		} catch (...) {}

		// Does the IfcElement have any IfcOpenings?
		// Note that openings for IfcOpeningElements are not processed
		IfcSchema::IfcRelVoidsElement::list openings = IfcSchema::IfcRelVoidsElement::list();
		if ( ifc_product->is(IfcSchema::Type::IfcElement) && !ifc_product->is(IfcSchema::Type::IfcOpeningElement) ) {
			IfcSchema::IfcElement::ptr element = reinterpret_pointer_cast<IfcSchema::IfcProduct,IfcSchema::IfcElement>(ifc_product);
			openings = element->HasOpenings();
		}
		// Is the IfcElement a decomposition of an IfcElement with any IfcOpeningElements?
		if ( ifc_product->is(IfcSchema::Type::IfcBuildingElementPart ) ) {
			IfcSchema::IfcBuildingElementPart::ptr part = reinterpret_pointer_cast<IfcSchema::IfcProduct,IfcSchema::IfcBuildingElementPart>(ifc_product);
#ifdef USE_IFC4
			IfcSchema::IfcRelAggregates::list decomposes = part->Decomposes();
			for ( IfcSchema::IfcRelAggregates::it it = decomposes->begin(); it != decomposes->end(); ++ it ) {
#else
			IfcSchema::IfcRelDecomposes::list decomposes = part->Decomposes();
			for ( IfcSchema::IfcRelDecomposes::it it = decomposes->begin(); it != decomposes->end(); ++ it ) {
#endif
				IfcSchema::IfcObjectDefinition::ptr obdef = (*it)->RelatingObject();
				if ( obdef->is(IfcSchema::Type::IfcElement) ) {
					IfcSchema::IfcElement::ptr element = reinterpret_pointer_cast<IfcSchema::IfcObjectDefinition,IfcSchema::IfcElement>(obdef);
					openings->push(element->HasOpenings());
				}
			}
		}

		if ( !disable_subtractions && openings && openings->Size() ) {
			IfcGeom::IfcRepresentationShapeItems opened_shapes;
			try {
				if ( use_faster_booleans ) {
					bool succes = IfcGeom::convert_openings_fast(ifc_product,openings,shapes,trsf,opened_shapes);
					if ( ! succes ) {
						opened_shapes.clear();
						IfcGeom::convert_openings(ifc_product,openings,shapes,trsf,opened_shapes);
					}
				} else {
					IfcGeom::convert_openings(ifc_product,openings,shapes,trsf,opened_shapes);
				}
			} catch(...) { 
				Logger::Message(Logger::LOG_ERROR,"Error processing openings for:",ifc_product->entity); 
			}
			if ( use_world_coords ) {
				for ( IfcGeom::IfcRepresentationShapeItems::iterator it = opened_shapes.begin(); it != opened_shapes.end(); ++ it ) {
					it->prepend(trsf);
				}
				trsf = gp_Trsf();
			}
			shape = new IfcGeomObjects::IfcRepresentationShapeModel(shaperep->entity->id(),opened_shapes);
		} else if ( use_world_coords ) {
			for ( IfcGeom::IfcRepresentationShapeItems::iterator it = shapes.begin(); it != shapes.end(); ++ it ) {
				it->prepend(trsf);
			}
			trsf = gp_Trsf();
			shape = new IfcGeomObjects::IfcRepresentationShapeModel(shaperep->entity->id(),shapes);
		} else {
			shape = new IfcGeomObjects::IfcRepresentationShapeModel(shaperep->entity->id(),shapes);
		}

		return new IfcGeomObjects::IfcGeomShapeModelObject(ifc_product->entity->id(), parent_id, name, 
			IfcSchema::Type::ToString(ifc_product->type()), guid, trsf, shape);
	}	
}

bool try_and_create_representations_for_current_entity() {
	current_shape_model_obj = create_shape_model_for_next_entity();
	if (current_shape_model_obj == 0) {
		return false;
	}
	
	if (use_brep_data) {
		current_brep_data_obj = new IfcGeomObjects::IfcGeomBrepDataObject(*current_shape_model_obj);
		if (current_brep_data_obj == 0) {
			return false;
		}
	}

	if (!disable_triangulation) {
		current_geom_obj = new IfcGeomObjects::IfcGeomObject(*current_shape_model_obj);
		if (current_geom_obj == 0) {
			return false;
		}
	}

	return true;
}

bool IfcGeomObjects::Next() {
	// Free all possible representations of the current geometrical entity
	delete current_geom_obj;
	delete current_brep_data_obj;
	delete current_shape_model_obj;
	current_geom_obj = 0;
	current_brep_data_obj = 0;
	current_shape_model_obj = 0;

	// Increment the iterator over the list of products using the current
	// shape representation
	if (entities) {
		++ifcproduct_iterator;
	}

	return try_and_create_representations_for_current_entity();
}

static std::vector<IfcGeomObjects::IfcObject*> returned_objects;
bool IfcGeomObjects::CleanUp() {
	// TODO: Correctly implement destructor for IfcFile
	delete ifc_file;
	IfcGeom::Cache::Purge();
	std::vector<IfcGeomObjects::IfcObject*>::const_iterator it;
	for (it = returned_objects.begin(); it != returned_objects.end(); ++ it ) {
		delete *it;
	}
	returned_objects.clear();
	return true;
}
const IfcGeomObjects::IfcObject* IfcGeomObjects::GetObject(int id) {
	IfcObject* ifc_object = 0;
	try {
		const IfcParse::IfcEntity& ifc_entity = ifc_file->EntityById(id);
		if ( ifc_entity->is(IfcSchema::Type::IfcProduct) ) {
			IfcSchema::IfcProduct::ptr ifc_product = reinterpret_pointer_cast<IfcUtil::IfcBaseClass,IfcSchema::IfcProduct>(ifc_entity);
			int parent_id = -1;
			try {
				parent_id = _getParentId(ifc_product);
			} catch (...) {}
			const std::string name = ifc_product->hasName() ? ifc_product->Name() : "";
			gp_Trsf trsf;
			try {
				IfcGeom::convert(ifc_product->ObjectPlacement(),trsf);
			} catch (...) {}
			ifc_object = new IfcObject(ifc_product->entity->id(),parent_id,name,
				IfcSchema::Type::ToString(ifc_product->type()),ifc_product->GlobalId(),trsf);
		}
	} catch(...) {}
	if ( !ifc_object ) ifc_object = new IfcObject(-1,-1,"","","",gp_Trsf());
	returned_objects.push_back(ifc_object);
	return ifc_object;
}
const IfcGeomObjects::IfcGeomObject* IfcGeomObjects::Get() {
	if (disable_triangulation) {
		throw std::runtime_error("No triangulation available");
	}
	return current_geom_obj;
}
const IfcGeomObjects::IfcGeomShapeModelObject* IfcGeomObjects::GetShapeModel() {
	return current_shape_model_obj;
}
const IfcGeomObjects::IfcGeomBrepDataObject* IfcGeomObjects::GetBrepData() {
	if (!use_brep_data) {
		throw std::runtime_error("No BRep data available");
	}
	return current_brep_data_obj;
}
double UnitPrefixToValue( IfcSchema::IfcSIPrefix::IfcSIPrefix v ) {
	if ( v == IfcSchema::IfcSIPrefix::IfcSIPrefix_EXA   ) return (double) 1e18;
	else if ( v == IfcSchema::IfcSIPrefix::IfcSIPrefix_PETA  ) return (double) 1e15;
	else if ( v == IfcSchema::IfcSIPrefix::IfcSIPrefix_TERA  ) return (double) 1e12;
	else if ( v == IfcSchema::IfcSIPrefix::IfcSIPrefix_GIGA  ) return (double) 1e9;
	else if ( v == IfcSchema::IfcSIPrefix::IfcSIPrefix_MEGA  ) return (double) 1e6;
	else if ( v == IfcSchema::IfcSIPrefix::IfcSIPrefix_KILO  ) return (double) 1e3;
	else if ( v == IfcSchema::IfcSIPrefix::IfcSIPrefix_HECTO ) return (double) 1e2;
	else if ( v == IfcSchema::IfcSIPrefix::IfcSIPrefix_DECA  ) return (double) 1;
	else if ( v == IfcSchema::IfcSIPrefix::IfcSIPrefix_DECI  ) return (double) 1e-1;
	else if ( v == IfcSchema::IfcSIPrefix::IfcSIPrefix_CENTI ) return (double) 1e-2;
	else if ( v == IfcSchema::IfcSIPrefix::IfcSIPrefix_MILLI ) return (double) 1e-3;
	else if ( v == IfcSchema::IfcSIPrefix::IfcSIPrefix_MICRO ) return (double) 1e-6;
	else if ( v == IfcSchema::IfcSIPrefix::IfcSIPrefix_NANO  ) return (double) 1e-9;
	else if ( v == IfcSchema::IfcSIPrefix::IfcSIPrefix_PICO  ) return (double) 1e-12;
	else if ( v == IfcSchema::IfcSIPrefix::IfcSIPrefix_FEMTO ) return (double) 1e-15;
	else if ( v == IfcSchema::IfcSIPrefix::IfcSIPrefix_ATTO  ) return (double) 1e-18;
	else return 1.0f;
}

void IfcGeomObjects::InitPrecision() {
	IfcSchema::IfcGeometricRepresentationContext::list rep_contexts = ifc_file->EntitiesByType<IfcSchema::IfcGeometricRepresentationContext>();
	// Currently, IfcGeometricRepresentationContext aren't used as much as they should be
	// in the evaluation of shape representations, hence, we try to find the one with the
	// lowest precision. Typically, a value of 1e-5 is encountered. This value is applied
	// to all TopoDS_Shapes generated by one of the IfcGeom::convert() functions.
	// TODO: Many of the empirically found tolerances should probably be substituted by
	// one that is defined in the model file.
	double lowest_precision_encountered = std::numeric_limits<double>::infinity();
	bool any_precision_encountered = false;
	for (IfcSchema::IfcGeometricRepresentationContext::it it = rep_contexts->begin(); it != rep_contexts->end(); ++it) {
		IfcSchema::IfcGeometricRepresentationContext* rep_context = *it;
		if (rep_context->is(IfcSchema::Type::IfcGeometricRepresentationSubContext)) continue;
		if (rep_context->hasPrecision()) {
			const double precision = rep_context->Precision();
			if (precision < lowest_precision_encountered) {
				any_precision_encountered = true;
				lowest_precision_encountered = precision;
			}
		}
	}
	if (any_precision_encountered) {
		IfcGeom::SetValue(IfcGeom::GV_PRECISION, lowest_precision_encountered);
	}
}

static std::string unit_name = "METER";
static float unit_magnitude = 1.0f;

void IfcGeomObjects::InitUnits() {
	// Set default units, set length to meters, angles to undefined
	IfcGeom::SetValue(IfcGeom::GV_LENGTH_UNIT,1.0);
	IfcGeom::SetValue(IfcGeom::GV_PLANEANGLE_UNIT,-1.0);
	
	IfcSchema::IfcUnitAssignment::list unit_assignments = ifc_file->EntitiesByType<IfcSchema::IfcUnitAssignment>();
	IfcUtil::IfcAbstractSelect::list units = IfcUtil::IfcAbstractSelect::list();
	if ( unit_assignments->Size() ) {
		IfcSchema::IfcUnitAssignment::ptr unit_assignment = *unit_assignments->begin();
		units = unit_assignment->Units();
	}
	if ( ! units ) {
		// No units eh... Since tolerances and deflection are specified internally in meters
		// we will try to find another indication of the model size.
		IfcSchema::IfcExtrudedAreaSolid::list extrusions = ifc_file->EntitiesByType<IfcSchema::IfcExtrudedAreaSolid>();
		if ( ! extrusions->Size() ) return;
		double max_height = -1.0f;
		for ( IfcSchema::IfcExtrudedAreaSolid::it it = extrusions->begin(); it != extrusions->end(); ++ it ) {
			const double depth = (*it)->Depth();
			if ( depth > max_height ) max_height = depth;
		}
		if ( max_height > 100.0f ) IfcGeom::SetValue(IfcGeom::GV_LENGTH_UNIT,0.001);
		return;
	}
	try {
		for ( IfcUtil::IfcAbstractSelect::it it = units->begin(); it != units->end(); ++ it ) {
			std::string current_unit_name = "";
			const IfcUtil::IfcAbstractSelect::ptr base = *it;
			IfcSchema::IfcSIUnit::ptr unit = IfcSchema::IfcSIUnit::ptr();
			double value = 1.0f;
			if ( base->is(IfcSchema::Type::IfcConversionBasedUnit) ) {
				const IfcSchema::IfcConversionBasedUnit::ptr u = reinterpret_pointer_cast<IfcUtil::IfcAbstractSelect,IfcSchema::IfcConversionBasedUnit>(base);
				current_unit_name = u->Name();
				const IfcSchema::IfcMeasureWithUnit::ptr u2 = u->ConversionFactor();
				IfcSchema::IfcUnit u3 = u2->UnitComponent();
				if ( u3->is(IfcSchema::Type::IfcSIUnit) ) {
					unit = (IfcSchema::IfcSIUnit*) u3;
				}
				IfcSchema::IfcValue v = u2->ValueComponent();
				IfcUtil::IfcArgumentSelect* v2 = (IfcUtil::IfcArgumentSelect*) v;
				const double f = *v2->wrappedValue();
				value *= f;
			} else if ( base->is(IfcSchema::Type::IfcSIUnit) ) {
				unit = reinterpret_pointer_cast<IfcUtil::IfcAbstractSelect,IfcSchema::IfcSIUnit>(base);
			}
			if ( unit ) {
				if ( unit->hasPrefix() ) {
					value *= UnitPrefixToValue(unit->Prefix());
				}
				IfcSchema::IfcUnitEnum::IfcUnitEnum type = unit->UnitType();
				if ( type == IfcSchema::IfcUnitEnum::IfcUnit_LENGTHUNIT ) {
					IfcGeom::SetValue(IfcGeom::GV_LENGTH_UNIT,value);
					if (current_unit_name.empty()) {
						if (unit->hasPrefix()) {
							current_unit_name = IfcSchema::IfcSIPrefix::ToString(unit->Prefix());
						}
						current_unit_name += IfcSchema::IfcSIUnitName::ToString(unit->Name());
					}
					unit_magnitude = value;
					unit_name = current_unit_name;
				} else if ( type == IfcSchema::IfcUnitEnum::IfcUnit_PLANEANGLEUNIT ) {
					IfcGeom::SetValue(IfcGeom::GV_PLANEANGLE_UNIT,value);
				}
			}
		}
	} catch ( IfcParse::IfcException ex ) {
		Logger::Message(Logger::LOG_ERROR,ex.what());
	}
}

bool IfcGeomObjects::Init(const std::string fn) {
	return IfcGeomObjects::Init(fn, 0, 0);
}
bool _Init() {
	IfcGeomObjects::InitUnits();
	IfcGeomObjects::InitPrecision();

	shapereps = ifc_file->EntitiesByType<IfcSchema::IfcShapeRepresentation>();
	if ( ! shapereps ) return false;
	
	shaperep_iterator = shapereps->begin();
	entities.reset();

	if (!try_and_create_representations_for_current_entity()) {
		return false;
	}

	done = 0;
	total = shapereps->Size();
	return true;
}
bool IfcGeomObjects::Init(const std::string fn, std::ostream* log1, std::ostream* log2) {
	Logger::SetOutput(log1,log2);
	ifc_file = new IfcParse::IfcFile();
	if ( !ifc_file->Init(fn) ) return false;
	return _Init();	
}
bool IfcGeomObjects::Init(std::istream& f, int len, std::ostream* log1, std::ostream* log2) {
	Logger::SetOutput(log1,log2);
	ifc_file = new IfcParse::IfcFile();
	if ( !ifc_file->Init(f, len) ) return false;
	return _Init();
}
bool IfcGeomObjects::Init(void* data, int len) {
	Logger::SetOutput(0,0);
	ifc_file = new IfcParse::IfcFile();
	if ( !ifc_file->Init(data, len) ) return false;
	return _Init();
}
void IfcGeomObjects::Settings(int setting, bool value) {
	switch ( setting ) {
	case USE_WORLD_COORDS:
		use_world_coords = value;
		break;
	case WELD_VERTICES:
		weld_vertices = value;
		break;
	case CONVERT_BACK_UNITS:
		convert_back_units = value;
		break;
	case USE_BREP_DATA:
		use_brep_data = value;
		break;
	case FASTER_BOOLEANS:
		use_faster_booleans = value;
		break;
	case SEW_SHELLS:
		IfcGeom::SetValue(IfcGeom::GV_MAX_FACES_TO_SEW,value ? 1000 : -1);
		break;
	case FORCE_CCW_FACE_ORIENTATION:
		IfcGeom::SetValue(IfcGeom::GV_FORCE_CCW_FACE_ORIENTATION,value ? 1 : -1);
		break;
    case DISABLE_OPENING_SUBTRACTIONS:
        disable_subtractions = value;
        break;
	case DISABLE_TRIANGULATION:
        disable_triangulation = value;
        break;
	}
}
int IfcGeomObjects::Progress() {
	return 100 * done / total;
}

const std::string& IfcGeomObjects::GetUnitName() {
	return unit_name;
}

const float IfcGeomObjects::GetUnitMagnitude() {
	return unit_magnitude;
}

const std::string IfcGeomObjects::GetLog() {
	return Logger::GetLog();
}

IfcParse::IfcFile* IfcGeomObjects::GetFile() {
  return ifc_file;
}

static double black[3] = {0,0,0};

IfcGeomObjects::Material::Material(const IfcGeom::SurfaceStyle* style) : style(style) {}
bool IfcGeomObjects::Material::hasDiffuse() const { return style->Diffuse(); }
bool IfcGeomObjects::Material::hasSpecular() const { return style->Specular(); }
bool IfcGeomObjects::Material::hasTransparency() const { return style->Transparency(); }
bool IfcGeomObjects::Material::hasSpecularity() const { return style->Specularity(); }
const double* IfcGeomObjects::Material::diffuse() const { if (hasDiffuse()) return &((*style->Diffuse()).R()); else return black; }
const double* IfcGeomObjects::Material::specular() const { if (hasSpecular()) return &((*style->Specular()).R()); else return black; }
double IfcGeomObjects::Material::transparency() const { if (hasTransparency()) return *style->Transparency(); else return 0; }
double IfcGeomObjects::Material::specularity() const { if (hasSpecularity()) return *style->Specularity(); else return 0; }
const std::string IfcGeomObjects::Material::name() const { return style->Name(); }
bool IfcGeomObjects::Material::operator==(const IfcGeomObjects::Material& other) const { return style == other.style; }

int IfcGeomObjects::IfcRepresentationBrepData::id() const { return _id; }
const std::string& IfcGeomObjects::IfcRepresentationBrepData::brep_data() const { return _brep_data; }

int IfcGeomObjects::IfcRepresentationTriangulation::id() const { return _id; }
const std::vector<float>& IfcGeomObjects::IfcRepresentationTriangulation::verts() const { return _verts; }
const std::vector<int>& IfcGeomObjects::IfcRepresentationTriangulation::faces() const { return _faces; }
const std::vector<int>& IfcGeomObjects::IfcRepresentationTriangulation::edges() const { return _edges; }
const std::vector<float>& IfcGeomObjects::IfcRepresentationTriangulation::normals() const { return _normals; }
const std::vector<int>& IfcGeomObjects::IfcRepresentationTriangulation::material_ids() const { return _material_ids; }
const std::vector<IfcGeomObjects::Material>& IfcGeomObjects::IfcRepresentationTriangulation::materials() const { return _materials; }

int IfcGeomObjects::IfcObject::id() const { return _id; }
int IfcGeomObjects::IfcObject::parent_id() const { return _parent_id; }
const std::string& IfcGeomObjects::IfcObject::name() const { return _name; }
const std::string& IfcGeomObjects::IfcObject::type() const { return _type; }
const std::string& IfcGeomObjects::IfcObject::guid() const { return _guid; }
const std::vector<float>& IfcGeomObjects::IfcObject::matrix() const { return _matrix; }

const IfcGeomObjects::IfcRepresentationShapeModel& IfcGeomObjects::IfcGeomShapeModelObject::mesh() const { return *_mesh; }
const IfcGeomObjects::IfcRepresentationTriangulation& IfcGeomObjects::IfcGeomObject::mesh() const { return *_mesh; }
const IfcGeomObjects::IfcRepresentationBrepData& IfcGeomObjects::IfcGeomBrepDataObject::mesh() const { return *_mesh; }

