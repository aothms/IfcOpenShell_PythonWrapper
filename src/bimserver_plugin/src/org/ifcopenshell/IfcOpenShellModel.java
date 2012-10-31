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
* This class communicates with the JNI wrapper of IfcOpenShell. Note that,     *
* contrary to the Bonsma IFC engine, if the wrapper crashes it will take the   *
* BIMserver down with her. Since loading the wrapper involves loading a        *
* considerable binary into memory, it would have been better to make the       *
* System.load() call somewhere in IfcOpenShellEngine.java.                     *
*                                                                              *
********************************************************************************/

package org.ifcopenshell;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Set;

import org.bimserver.plugins.ifcengine.IfcEngineClash;
import org.bimserver.plugins.ifcengine.IfcEngineException;
import org.bimserver.plugins.ifcengine.IfcEngineGeometry;
import org.bimserver.plugins.ifcengine.IfcEngineInstance;
import org.bimserver.plugins.ifcengine.IfcEngineModel;
import org.bimserver.plugins.ifcengine.IfcEngineSurfaceProperties;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class IfcOpenShellModel implements IfcEngineModel {
	private static final Logger LOGGER = LoggerFactory.getLogger(IfcOpenShellModel.class);
	private Boolean validModel = false;
	private HashMap<String,List<IfcOpenShellEntityInstance>> instances;
	private HashMap<Integer,IfcOpenShellEntityInstance> instances_byid;
	
	// Native C++ functions implemented in IfcOpenShell
	private native IfcGeomObject getGeometry();
	private native boolean setIfcData(byte[] b);
	private native String getPluginVersion();

	// Convenience functions to concatenate arrays
	private static float[] concatFloatArray(float[] a, float[] b) {
		if ( a == null ) return b;
		float[] r = new float[a.length+b.length];
		System.arraycopy(a,0,r,0,a.length);
		System.arraycopy(b,0,r,a.length,b.length);
		return r;
	}	
	private static int[] concatIntArray(int[] a, int[] b) {
		if ( a == null ) return b;
		int[] r = new int[a.length+b.length];
		System.arraycopy(a,0,r,0,a.length);
		System.arraycopy(b,0,r,a.length,b.length);
		return r;
	}
	
	// Load the binary and pass the IFC data to IfcOpenShell
	public IfcOpenShellModel(String fn, byte[] input) throws IfcEngineException {
		try {
			System.load(fn);
		} catch ( Throwable e ) {
			e.printStackTrace();
			throw new IfcEngineException("Failed to load IfcOpenShell library");
		}
		String java_version = IfcOpenShellEnginePlugin.getVersionStatic();
		String cpp_version = "";
		try {
			cpp_version = getPluginVersion();
		} catch ( UnsatisfiedLinkError e ) {
			throw new IfcEngineException("Unable to determine IfcOpenShell version");
		}
		if ( !java_version.equalsIgnoreCase(cpp_version) ) {
			throw new IfcEngineException(String.format("Version mismatch: Plugin version %s does not match IfcOpenShell version %s",java_version,cpp_version));
		}
		validModel = setIfcData(input);
	}

	@Override
	public void close() throws IfcEngineException {
		validModel = false;
	}

	@Override
	public IfcEngineGeometry finalizeModelling(
			IfcEngineSurfaceProperties surfaceProperties)
			throws IfcEngineException {
		
		if ( ! validModel ) throw new IfcEngineException("No valid model supplied");
		
		int[] indices = null;
		float[] normals = null;
		float[] vertices = null;
		
		// We keep track of instances ourselves
		instances = new HashMap<String,List<IfcOpenShellEntityInstance>>();
		
		instances_byid = new HashMap<Integer,IfcOpenShellEntityInstance>();
		
		while ( true ) {
			// Get the geometry from IfcOpenShell, this goes one instance at a time
			IfcGeomObject obj = getGeometry();
			if (obj == null) break;
			
			// Store the instance in our dictionary
			List<IfcOpenShellEntityInstance> entity_list;
			String type = obj.type.toUpperCase();
			if ( ! instances.containsKey(type) ) {
				entity_list = new ArrayList<IfcOpenShellEntityInstance>();
				instances.put(type, entity_list);
			} else {
				entity_list = instances.get(type);
			}
			
			int start_vertex = vertices == null ? 0 : vertices.length;
			int start_index = indices == null ? 0 : indices.length;
			int fcount = obj.indices.length / 3;
			
			// The indices have to take previous entities into account
			for ( int i = 0; i < obj.indices.length; ++ i ) {
				obj.indices[i] += start_vertex / 3;
			}
			
			indices = concatIntArray(indices,obj.indices);
			vertices = concatFloatArray(vertices,obj.vertices);
			normals = concatFloatArray(normals,obj.normals);
			
			IfcOpenShellEntityInstance instance = new IfcOpenShellEntityInstance(start_vertex,start_index,fcount);
			entity_list.add(instance);			
			instances_byid.put(obj.id, instance);
		}
		return new IfcEngineGeometry(indices,vertices,normals);
	}

	@Override
	public Set<IfcEngineClash> findClashesWithEids(double d)
			throws IfcEngineException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public Set<IfcEngineClash> findClashesWithGuids(double d)
			throws IfcEngineException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public List<? extends IfcEngineInstance> getInstances(String name)
			throws IfcEngineException {
		if ( instances.containsKey(name)) return instances.get(name);
		else return new ArrayList<IfcOpenShellEntityInstance>();
	}

	@Override
	public IfcEngineSurfaceProperties initializeModelling()
			throws IfcEngineException {
		return null;
	}

	@Override
	public void setPostProcessing(boolean postProcessing)
			throws IfcEngineException {
	}
	@Override
	public IfcEngineInstance getInstanceFromExpressId(int oid)
			throws IfcEngineException {
		if ( instances_byid.containsKey(oid) ) {
			return instances_byid.get(oid);
		} else {
			// Probably something went wrong with the processing of this element in
			// the IfcOpenShell binary, as it has not been included in the enumerated
			// set of elements with geometry.
			LOGGER.warn(String.format("Entity #%d not found in model", oid));
			return new IfcOpenShellEntityInstance(0,0,0);			
		}
	}
	@Override
	public void setFormat(int format, int mask) throws IfcEngineException {
	}

}
