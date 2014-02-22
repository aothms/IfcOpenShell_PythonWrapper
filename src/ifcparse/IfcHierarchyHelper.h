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

#ifndef IFCHIERARCHYHELPER_H
#define IFCHIERARCHYHELPER_H

#ifdef USE_IFC4
#include "../ifcparse/Ifc4.h"
#else
#include "../ifcparse/Ifc2x3.h"
#endif
#include "../ifcparse/IfcFile.h"
#include "../ifcparse/IfcSpfStream.h"
#include "../ifcparse/IfcWrite.h"

class IfcHierarchyHelper : public IfcParse::IfcFile {
public:
	template <class T> 
	T* addTriplet(double x, double y, double z) {
		std::vector<double> a; a.push_back(x); a.push_back(y); a.push_back(z);
		T* t = new T(a);
		AddEntity(t);
		return t;
	}

	template <class T> 
	T* addDoublet(double x, double y) {
		std::vector<double> a; a.push_back(x); a.push_back(y);
		T* t = new T(a);
		AddEntity(t);
		return t;
	}

	template <class T>
	T* getSingle() {
		typename T::list ts = EntitiesByType<T>();
		if (ts->Size() != 1) return 0;
		return *ts->begin();
	}
	
	IfcSchema::IfcAxis2Placement3D* addPlacement3d(double ox=0.0, double oy=0.0, double oz=0.0,
		double zx=0.0, double zy=0.0, double zz=1.0,
		double xx=1.0, double xy=0.0, double xz=0.0);

	IfcSchema::IfcAxis2Placement2D* addPlacement2d(double ox=0.0, double oy=0.0,
		double xx=1.0, double xy=0.0);

	IfcSchema::IfcLocalPlacement* addLocalPlacement(double ox=0.0, double oy=0.0, double oz=0.0,
		double zx=0.0, double zy=0.0, double zz=1.0,
		double xx=1.0, double xy=0.0, double xz=0.0);

	template <class T>
	void addRelatedObject(IfcSchema::IfcObjectDefinition* related_object, 
		IfcSchema::IfcObjectDefinition* relating_object, IfcSchema::IfcOwnerHistory* owner_hist = 0)
	{
		typename T::list li = EntitiesByType<T>();
		bool found = false;
		for (typename T::it i = li->begin(); i != li->end(); ++i) {
			T* rel = *i;
			if (rel->RelatingObject() == relating_object) {
				IfcSchema::IfcObjectDefinition::list products = rel->RelatedObjects();
				products->push(related_object);
				rel->setRelatedObjects(products);
				found = true;
				break;
			}
		}
		if (! found) {
			if (! owner_hist) {
				owner_hist = getSingle<IfcSchema::IfcOwnerHistory>();
			}
			if (! owner_hist) {
				owner_hist = addOwnerHistory();
			}
			IfcSchema::IfcObjectDefinition::list relating_objects (new IfcTemplatedEntityList<IfcSchema::IfcObjectDefinition>());
			relating_objects->push(relating_object);
			T* t = new T(IfcWrite::IfcGuidHelper(), owner_hist, boost::none, boost::none, related_object, relating_objects);
			AddEntity(t);
		}
	}

	IfcSchema::IfcOwnerHistory* addOwnerHistory();	
	IfcSchema::IfcProject* addProject(IfcSchema::IfcOwnerHistory* owner_hist = 0);
	void relatePlacements(IfcSchema::IfcProduct* parent, IfcSchema::IfcProduct* product);	
	IfcSchema::IfcSite* addSite(IfcSchema::IfcProject* proj = 0, IfcSchema::IfcOwnerHistory* owner_hist = 0);	
	IfcSchema::IfcBuilding* addBuilding(IfcSchema::IfcSite* site = 0, IfcSchema::IfcOwnerHistory* owner_hist = 0);

	IfcSchema::IfcBuildingStorey* addBuildingStorey(IfcSchema::IfcBuilding* building = 0, 
		IfcSchema::IfcOwnerHistory* owner_hist = 0);

	IfcSchema::IfcBuildingStorey* addBuildingProduct(IfcSchema::IfcProduct* product, 
		IfcSchema::IfcBuildingStorey* storey = 0, IfcSchema::IfcOwnerHistory* owner_hist = 0);

	void addExtrudedPolyline(IfcSchema::IfcShapeRepresentation* rep, const std::vector<std::pair<double, double> >& points, double h, 
		IfcSchema::IfcAxis2Placement2D* place=0, IfcSchema::IfcAxis2Placement3D* place2=0, 
		IfcSchema::IfcDirection* dir=0, IfcSchema::IfcRepresentationContext* context=0);

	IfcSchema::IfcProductDefinitionShape* addExtrudedPolyline(const std::vector<std::pair<double, double> >& points, double h, 
		IfcSchema::IfcAxis2Placement2D* place=0, IfcSchema::IfcAxis2Placement3D* place2=0, IfcSchema::IfcDirection* dir=0, 
		IfcSchema::IfcRepresentationContext* context=0);

	void addBox(IfcSchema::IfcShapeRepresentation* rep, double w, double d, double h, 
		IfcSchema::IfcAxis2Placement2D* place=0, IfcSchema::IfcAxis2Placement3D* place2=0, 
		IfcSchema::IfcDirection* dir=0, IfcSchema::IfcRepresentationContext* context=0);

	IfcSchema::IfcProductDefinitionShape* addBox(double w, double d, double h, IfcSchema::IfcAxis2Placement2D* place=0, 
		IfcSchema::IfcAxis2Placement3D* place2=0, IfcSchema::IfcDirection* dir=0, IfcSchema::IfcRepresentationContext* context=0);

	void clipRepresentation(IfcSchema::IfcProductRepresentation* shape, 
		IfcSchema::IfcAxis2Placement3D* place, bool agree);

	IfcSchema::IfcPresentationStyleAssignment* setSurfaceColour(IfcSchema::IfcProductRepresentation* shape, 
		double r, double g, double b, double a=1.0);

	void setSurfaceColour(IfcSchema::IfcProductRepresentation* shape, 
		IfcSchema::IfcPresentationStyleAssignment* style_assignment);

};

template <>
inline void IfcHierarchyHelper::addRelatedObject <IfcSchema::IfcRelContainedInSpatialStructure> (IfcSchema::IfcObjectDefinition* related_object, 
	IfcSchema::IfcObjectDefinition* relating_object, IfcSchema::IfcOwnerHistory* owner_hist)
{
	IfcSchema::IfcRelContainedInSpatialStructure::list li = EntitiesByType<IfcSchema::IfcRelContainedInSpatialStructure>();
	bool found = false;
	for (IfcSchema::IfcRelContainedInSpatialStructure::it i = li->begin(); i != li->end(); ++i) {
		IfcSchema::IfcRelContainedInSpatialStructure* rel = *i;
		if (rel->RelatingStructure() == relating_object) {
			IfcSchema::IfcProduct::list products = rel->RelatedElements();
			products->push((IfcSchema::IfcProduct*)related_object);
			rel->setRelatedElements(products);
			found = true;
			break;
		}
	}
	if (! found) {
		if (! owner_hist) {
			owner_hist = getSingle<IfcSchema::IfcOwnerHistory>();
		}
		if (! owner_hist) {
			owner_hist = addOwnerHistory();
		}
		IfcSchema::IfcProduct::list relating_objects (new IfcTemplatedEntityList<IfcSchema::IfcProduct>());
		relating_objects->push((IfcSchema::IfcProduct*)relating_object);
		IfcSchema::IfcRelContainedInSpatialStructure* t = new IfcSchema::IfcRelContainedInSpatialStructure(IfcWrite::IfcGuidHelper(), owner_hist, 
			boost::none, boost::none, relating_objects, (IfcSchema::IfcSpatialStructureElement*)related_object);

		AddEntity(t);
	}
}

#endif