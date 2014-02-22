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

/********************************************************************************
 *                                                                              *
 * This class is a subclass of the regular IfcFile class that implements        *
 * several convenience functions for creating geometrical representations and   *
 * spatial containers.                                                          *
 *                                                                              *
 ********************************************************************************/

#include <time.h>

#include "../ifcparse/IfcHierarchyHelper.h"
	
IfcSchema::IfcAxis2Placement3D* IfcHierarchyHelper::addPlacement3d(
	double ox, double oy, double oz,
	double zx, double zy, double zz,
	double xx, double xy, double xz) 
{
		IfcSchema::IfcDirection* x = addTriplet<IfcSchema::IfcDirection>(xx, xy, xz);
		IfcSchema::IfcDirection* z = addTriplet<IfcSchema::IfcDirection>(zx, zy, zz);
		IfcSchema::IfcCartesianPoint* o = addTriplet<IfcSchema::IfcCartesianPoint>(ox, oy, oz);
		IfcSchema::IfcAxis2Placement3D* p3d = new IfcSchema::IfcAxis2Placement3D(o, z, x);
		AddEntity(p3d);
		return p3d;
}

IfcSchema::IfcAxis2Placement2D* IfcHierarchyHelper::addPlacement2d(
	double ox, double oy,
	double xx, double xy) 
{
		IfcSchema::IfcDirection* x = addDoublet<IfcSchema::IfcDirection>(xx, xy);
		IfcSchema::IfcCartesianPoint* o = addDoublet<IfcSchema::IfcCartesianPoint>(ox, oy);
		IfcSchema::IfcAxis2Placement2D* p2d = new IfcSchema::IfcAxis2Placement2D(o, x);
		AddEntity(p2d);
		return p2d;
}

IfcSchema::IfcLocalPlacement* IfcHierarchyHelper::addLocalPlacement(
	double ox, double oy, double oz,
	double zx, double zy, double zz,
	double xx, double xy, double xz) 
{
		IfcSchema::IfcLocalPlacement* lp = new IfcSchema::IfcLocalPlacement(0, 
			addPlacement3d(ox, oy, oz, zx, zy, zz, xx, xy, xz));

		AddEntity(lp);
		return lp;
}

IfcSchema::IfcOwnerHistory* IfcHierarchyHelper::addOwnerHistory() {
	IfcSchema::IfcPerson* person = new IfcSchema::IfcPerson(boost::none, boost::none, std::string(""), 
		boost::none, boost::none, boost::none, boost::none, boost::none);

	IfcSchema::IfcOrganization* organization = new IfcSchema::IfcOrganization(boost::none, 
		"IfcOpenShell", boost::none, boost::none, boost::none);

	IfcSchema::IfcPersonAndOrganization* person_and_org = new IfcSchema::IfcPersonAndOrganization(person, organization, boost::none);
	IfcSchema::IfcApplication* application = new IfcSchema::IfcApplication(organization, 
		IFCOPENSHELL_VERSION, "IfcOpenShell", "IfcOpenShell");

	int timestamp = (int) time(0);
	IfcSchema::IfcOwnerHistory* owner_hist = new IfcSchema::IfcOwnerHistory(person_and_org, application, 
		boost::none, IfcSchema::IfcChangeActionEnum::IfcChangeAction_ADDED, boost::none, person_and_org, application, timestamp);

	AddEntity(person);
	AddEntity(organization);
	AddEntity(person_and_org);
	AddEntity(application);
	AddEntity(owner_hist);

	return owner_hist;
}
	
IfcSchema::IfcProject* IfcHierarchyHelper::addProject(IfcSchema::IfcOwnerHistory* owner_hist) {
	IfcSchema::IfcRepresentationContext::list rep_contexts (new IfcTemplatedEntityList<IfcSchema::IfcRepresentationContext>());
	IfcSchema::IfcGeometricRepresentationContext* rep_context = new IfcSchema::IfcGeometricRepresentationContext(
		std::string("Plan"), std::string("Model"), 3, 1e-5, addPlacement3d(), addTriplet<IfcSchema::IfcDirection>(0, 1, 0));

	rep_contexts->push(rep_context);

	IfcEntities units (new IfcEntityList());
	IfcSchema::IfcDimensionalExponents* dimexp = new IfcSchema::IfcDimensionalExponents(0, 0, 0, 0, 0, 0, 0);
	IfcSchema::IfcSIUnit* unit1 = new IfcSchema::IfcSIUnit(IfcSchema::IfcUnitEnum::IfcUnit_LENGTHUNIT, 
		IfcSchema::IfcSIPrefix::IfcSIPrefix_MILLI, IfcSchema::IfcSIUnitName::IfcSIUnitName_METRE);
	IfcSchema::IfcSIUnit* unit2a = new IfcSchema::IfcSIUnit(IfcSchema::IfcUnitEnum::IfcUnit_PLANEANGLEUNIT, 
		boost::none, IfcSchema::IfcSIUnitName::IfcSIUnitName_RADIAN);
	IfcSchema::IfcMeasureWithUnit* unit2b = new IfcSchema::IfcMeasureWithUnit(
		new IfcWrite::IfcSelectHelper(0.017453293, IfcSchema::Type::IfcPlaneAngleMeasure), unit2a);
	IfcSchema::IfcConversionBasedUnit* unit2 = new IfcSchema::IfcConversionBasedUnit(dimexp, 
		IfcSchema::IfcUnitEnum::IfcUnit_PLANEANGLEUNIT, "Degrees", unit2b);

	units->push(unit1);
	units->push(unit2);

	IfcSchema::IfcUnitAssignment* unit_assignment = new IfcSchema::IfcUnitAssignment(units);

	IfcSchema::IfcProject* project = new IfcSchema::IfcProject(IfcWrite::IfcGuidHelper(), owner_hist, boost::none, boost::none, 
		boost::none, boost::none, boost::none, rep_contexts, unit_assignment);

	AddEntity(rep_context);
	AddEntity(dimexp);
	AddEntity(unit1);
	AddEntity(unit2a);
	AddEntity(unit2b);
	AddEntity(unit2);
	AddEntity(unit_assignment);
	AddEntity(project);

	return project;
}

void IfcHierarchyHelper::relatePlacements(IfcSchema::IfcProduct* parent, IfcSchema::IfcProduct* product) {
	IfcSchema::IfcObjectPlacement* place = product->hasObjectPlacement() ? product->ObjectPlacement() : 0;
	if (place && place->is(IfcSchema::Type::IfcLocalPlacement)) {
		IfcSchema::IfcLocalPlacement* local_place = (IfcSchema::IfcLocalPlacement*) place;
		if (parent->hasObjectPlacement()) {
			local_place->setPlacementRelTo(parent->ObjectPlacement());
		}
	}
}
	
IfcSchema::IfcSite* IfcHierarchyHelper::addSite(IfcSchema::IfcProject* proj, IfcSchema::IfcOwnerHistory* owner_hist) {
	if (! owner_hist) {
		owner_hist = getSingle<IfcSchema::IfcOwnerHistory>();
	}
	if (! owner_hist) {
		owner_hist = addOwnerHistory();
	}
	if (! proj) {
		proj = getSingle<IfcSchema::IfcProject>();
	}
	if (! proj) {
		proj = addProject(owner_hist);
	}

	IfcSchema::IfcSite* site = new IfcSchema::IfcSite(IfcWrite::IfcGuidHelper(), owner_hist, boost::none, 
		boost::none, boost::none, addLocalPlacement(), 0, boost::none, 
		IfcSchema::IfcElementCompositionEnum::IfcElementComposition_ELEMENT, 
		boost::none, boost::none, boost::none, boost::none, 0);

	AddEntity(site);
	addRelatedObject<IfcSchema::IfcRelAggregates>(proj, site);
	return site;
}
	
IfcSchema::IfcBuilding* IfcHierarchyHelper::addBuilding(IfcSchema::IfcSite* site, IfcSchema::IfcOwnerHistory* owner_hist) {
	if (! owner_hist) {
		owner_hist = getSingle<IfcSchema::IfcOwnerHistory>();
	}
	if (! owner_hist) {
		owner_hist = addOwnerHistory();
	}
	if (! site) {
		site = getSingle<IfcSchema::IfcSite>();
	}
	if (! site) {
		site = addSite(0, owner_hist);
	}
	IfcSchema::IfcBuilding* building = new IfcSchema::IfcBuilding(IfcWrite::IfcGuidHelper(), owner_hist, boost::none, boost::none, boost::none, 
		addLocalPlacement(), 0, boost::none, IfcSchema::IfcElementCompositionEnum::IfcElementComposition_ELEMENT, 
		boost::none, boost::none, 0);

	AddEntity(building);
	addRelatedObject<IfcSchema::IfcRelAggregates>(site, building);
	relatePlacements(site, building);

	return building;
}

IfcSchema::IfcBuildingStorey* IfcHierarchyHelper::addBuildingStorey(IfcSchema::IfcBuilding* building, 
	IfcSchema::IfcOwnerHistory* owner_hist) 
{
	if (! owner_hist) {
		owner_hist = getSingle<IfcSchema::IfcOwnerHistory>();
	}
	if (! owner_hist) {
		owner_hist = addOwnerHistory();
	}
	if (! building) {
		building = getSingle<IfcSchema::IfcBuilding>();
	}
	if (! building) {
		building = addBuilding(0, owner_hist);
	}
	IfcSchema::IfcBuildingStorey* storey = new IfcSchema::IfcBuildingStorey(IfcWrite::IfcGuidHelper(), 
		owner_hist, boost::none, boost::none, boost::none, addLocalPlacement(), 0, boost::none, 
		IfcSchema::IfcElementCompositionEnum::IfcElementComposition_ELEMENT, boost::none);

	AddEntity(storey);
	addRelatedObject<IfcSchema::IfcRelAggregates>(building, storey);
	relatePlacements(building, storey);

	return storey;
}

IfcSchema::IfcBuildingStorey* IfcHierarchyHelper::addBuildingProduct(IfcSchema::IfcProduct* product, 
	IfcSchema::IfcBuildingStorey* storey, IfcSchema::IfcOwnerHistory* owner_hist) 
{
	if (! owner_hist) {
		owner_hist = getSingle<IfcSchema::IfcOwnerHistory>();
	}
	if (! owner_hist) {
		owner_hist = addOwnerHistory();
	}
	if (! storey) {
		storey = getSingle<IfcSchema::IfcBuildingStorey>();
	}
	if (! storey) {
		storey = addBuildingStorey(0, owner_hist);
	}
	AddEntity(product);
	addRelatedObject<IfcSchema::IfcRelContainedInSpatialStructure>(storey, product);
	relatePlacements(storey, product);
	return storey;
}

void IfcHierarchyHelper::addExtrudedPolyline(IfcSchema::IfcShapeRepresentation* rep, const std::vector<std::pair<double, double> >& points, double h, 
	IfcSchema::IfcAxis2Placement2D* place, IfcSchema::IfcAxis2Placement3D* place2, 
	IfcSchema::IfcDirection* dir, IfcSchema::IfcRepresentationContext* context) 
{
	IfcSchema::IfcCartesianPoint::list cartesian_points (new IfcTemplatedEntityList<IfcSchema::IfcCartesianPoint>());
	for (std::vector<std::pair<double, double> >::const_iterator i = points.begin(); i != points.end(); ++i) {
		cartesian_points->push(addDoublet<IfcSchema::IfcCartesianPoint>(i->first, i->second));
	}
	if (cartesian_points->Size()) cartesian_points->push(*cartesian_points->begin());
	IfcSchema::IfcPolyline* line = new IfcSchema::IfcPolyline(cartesian_points);
	IfcSchema::IfcArbitraryClosedProfileDef* profile = new IfcSchema::IfcArbitraryClosedProfileDef(
		IfcSchema::IfcProfileTypeEnum::IfcProfileType_AREA, boost::none, line);

	IfcSchema::IfcExtrudedAreaSolid* solid = new IfcSchema::IfcExtrudedAreaSolid(
		profile, place2 ? place2 : addPlacement3d(), dir ? dir : addTriplet<IfcSchema::IfcDirection>(0, 0, 1), h);

	IfcSchema::IfcRepresentationItem::list items = rep->Items();
	items->push(solid);
	rep->setItems(items);

	AddEntity(line);
	AddEntity(profile);
	AddEntity(solid);
}

IfcSchema::IfcProductDefinitionShape* IfcHierarchyHelper::addExtrudedPolyline(const std::vector<std::pair<double, double> >& points, double h, 
	IfcSchema::IfcAxis2Placement2D* place, IfcSchema::IfcAxis2Placement3D* place2, IfcSchema::IfcDirection* dir, 
	IfcSchema::IfcRepresentationContext* context) 
{
	IfcSchema::IfcRepresentation::list reps (new IfcTemplatedEntityList<IfcSchema::IfcRepresentation>());
	IfcSchema::IfcRepresentationItem::list items (new IfcTemplatedEntityList<IfcSchema::IfcRepresentationItem>());
	IfcSchema::IfcShapeRepresentation* rep = new IfcSchema::IfcShapeRepresentation(context 
		? context 
		: getSingle<IfcSchema::IfcRepresentationContext>(), std::string("Body"), std::string("SweptSolid"), items);
	reps->push(rep);
	IfcSchema::IfcProductDefinitionShape* shape = new IfcSchema::IfcProductDefinitionShape(0, 0, reps);		
	AddEntity(rep);
	AddEntity(shape);
	addExtrudedPolyline(rep, points, h, place, place2, dir, context);

	return shape;
}

void IfcHierarchyHelper::addBox(IfcSchema::IfcShapeRepresentation* rep, double w, double d, double h, 
	IfcSchema::IfcAxis2Placement2D* place, IfcSchema::IfcAxis2Placement3D* place2, 
	IfcSchema::IfcDirection* dir, IfcSchema::IfcRepresentationContext* context) 
{
	if (false) {
		IfcSchema::IfcRectangleProfileDef* profile = new IfcSchema::IfcRectangleProfileDef(
			IfcSchema::IfcProfileTypeEnum::IfcProfileType_AREA, 0, place ? place : addPlacement2d(), w, d);
		IfcSchema::IfcExtrudedAreaSolid* solid = new IfcSchema::IfcExtrudedAreaSolid(profile, 
			place2 ? place2 : addPlacement3d(), dir ? dir : addTriplet<IfcSchema::IfcDirection>(0, 0, 1), h);

		AddEntity(profile);
		AddEntity(solid);
		IfcSchema::IfcRepresentationItem::list items = rep->Items();
		items->push(solid);
		rep->setItems(items);
	} else {
		std::vector<std::pair<double, double> > points;
		points.push_back(std::pair<double, double>(-w/2, -d/2));
		points.push_back(std::pair<double, double>(w/2, -d/2));
		points.push_back(std::pair<double, double>(w/2, d/2));
		points.push_back(std::pair<double, double>(-w/2, d/2));
		points.push_back(*points.begin());
		addExtrudedPolyline(rep, points, h, place, place2, dir, context);
	}
}

IfcSchema::IfcProductDefinitionShape* IfcHierarchyHelper::addBox(double w, double d, double h, IfcSchema::IfcAxis2Placement2D* place, 
	IfcSchema::IfcAxis2Placement3D* place2, IfcSchema::IfcDirection* dir, IfcSchema::IfcRepresentationContext* context) 
{
	IfcSchema::IfcRepresentation::list reps (new IfcTemplatedEntityList<IfcSchema::IfcRepresentation>());
	IfcSchema::IfcRepresentationItem::list items (new IfcTemplatedEntityList<IfcSchema::IfcRepresentationItem>());		
	IfcSchema::IfcShapeRepresentation* rep = new IfcSchema::IfcShapeRepresentation(
		context ? context : getSingle<IfcSchema::IfcRepresentationContext>(), std::string("Body"), std::string("SweptSolid"), items);
	reps->push(rep);
	IfcSchema::IfcProductDefinitionShape* shape = new IfcSchema::IfcProductDefinitionShape(0, 0, reps);		
	AddEntity(rep);
	AddEntity(shape);
	addBox(rep, w, d, h, place, place2, dir, context);
	return shape;
}

void IfcHierarchyHelper::clipRepresentation(IfcSchema::IfcProductRepresentation* shape, 
	IfcSchema::IfcAxis2Placement3D* place, bool agree) 
{
	IfcSchema::IfcPlane* plane = new IfcSchema::IfcPlane(place);
	IfcSchema::IfcHalfSpaceSolid* half_space = new IfcSchema::IfcHalfSpaceSolid(plane, agree);
	IfcSchema::IfcRepresentation::list reps = shape->Representations();
	for (IfcSchema::IfcRepresentation::it j = reps->begin(); j != reps->end(); ++j) {
		IfcSchema::IfcRepresentation* rep = *j;
		if (rep->RepresentationIdentifier() != "Body") continue;
		rep->setRepresentationType("Clipping");		
		IfcSchema::IfcRepresentationItem::list items = rep->Items();
		IfcSchema::IfcRepresentationItem::list new_items (new IfcTemplatedEntityList<IfcSchema::IfcRepresentationItem>());
		AddEntity(plane);
		AddEntity(half_space);
		for (IfcSchema::IfcRepresentationItem::it i = items->begin(); i != items->end(); ++i) {
			IfcSchema::IfcRepresentationItem* item = *i;
			IfcSchema::IfcBooleanClippingResult* clip = new IfcSchema::IfcBooleanClippingResult(
				IfcSchema::IfcBooleanOperator::IfcBooleanOperator_DIFFERENCE, item, half_space);
			AddEntity(clip);
			new_items->push(clip);
		}
		rep->setItems(new_items);
	}
}

IfcSchema::IfcPresentationStyleAssignment* IfcHierarchyHelper::setSurfaceColour(
	IfcSchema::IfcProductRepresentation* shape, double r, double g, double b, double a) 
{
	IfcSchema::IfcColourRgb* colour = new IfcSchema::IfcColourRgb(boost::none, r, g, b);
	IfcSchema::IfcSurfaceStyleRendering* rendering = a == 1.0
		? new IfcSchema::IfcSurfaceStyleRendering(colour, boost::none, boost::none, boost::none, boost::none, boost::none, 
			boost::none, boost::none, IfcSchema::IfcReflectanceMethodEnum::IfcReflectanceMethod_FLAT)
		: new IfcSchema::IfcSurfaceStyleRendering(colour, 1.0-a, boost::none, boost::none, boost::none, boost::none, 
			boost::none, boost::none, IfcSchema::IfcReflectanceMethodEnum::IfcReflectanceMethod_FLAT);

	IfcEntities styles(new IfcEntityList());
	styles->push(rendering);
	IfcSchema::IfcSurfaceStyle* surface_style = new IfcSchema::IfcSurfaceStyle(
		boost::none, IfcSchema::IfcSurfaceSide::IfcSurfaceSide_BOTH, styles);
	IfcEntities surface_styles(new IfcEntityList());
	surface_styles->push(surface_style);
	IfcSchema::IfcPresentationStyleAssignment* style_assignment = 
		new IfcSchema::IfcPresentationStyleAssignment(surface_styles);
	AddEntity(colour);
	AddEntity(rendering);
	AddEntity(surface_style);
	AddEntity(style_assignment);
	setSurfaceColour(shape, style_assignment);
	return style_assignment;
}

void IfcHierarchyHelper::setSurfaceColour(IfcSchema::IfcProductRepresentation* shape, 
	IfcSchema::IfcPresentationStyleAssignment* style_assignment) 
{
#ifdef USE_IFC4 
	IfcEntities style_assignments (new IfcEntityList());
#else
	IfcSchema::IfcPresentationStyleAssignment::list style_assignments (new IfcTemplatedEntityList<IfcSchema::IfcPresentationStyleAssignment>());
#endif
	style_assignments->push(style_assignment);
	IfcSchema::IfcRepresentation::list reps = shape->Representations();
	for (IfcSchema::IfcRepresentation::it j = reps->begin(); j != reps->end(); ++j) {
		IfcSchema::IfcRepresentation* rep = *j;
		if (rep->RepresentationIdentifier() != "Body" && rep->RepresentationIdentifier() != "Facetation") continue;
		IfcSchema::IfcRepresentationItem::list items = rep->Items();
		for (IfcSchema::IfcRepresentationItem::it i = items->begin(); i != items->end(); ++i) {
			IfcSchema::IfcRepresentationItem* item = *i;
			IfcSchema::IfcStyledItem* styled_item = new IfcSchema::IfcStyledItem(item, style_assignments, boost::none);
			AddEntity(styled_item);
		}
	}
}
