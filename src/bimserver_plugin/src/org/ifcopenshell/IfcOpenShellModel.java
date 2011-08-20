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

package org.ifcopenshell;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
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
	private IfcOpenShellEngine engine;
	private Boolean validModel = false;
	private HashMap<String,List<IfcOpenShellEntityInstance>> instances;
	
	public IfcOpenShellModel(File input, IfcOpenShellEngine p) {
		engine = p;
		try {
			String r = engine.sendCommandText(String.format("load %s", input.getAbsolutePath()));
			validModel = "y".equals(r);
			validModel = true;
		} catch (IOException e) {
			e.printStackTrace();
		}
		
	}
	
	public IfcOpenShellModel(IfcOpenShellEngine p) {
		engine = p;
		validModel = true;
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
		
		List<Integer> indices_list = new ArrayList<Integer>();
		List<Number> normals_list = new ArrayList<Number>();
		List<Number> vertices_list = new ArrayList<Number>();
		
		instances = new HashMap<String,List<IfcOpenShellEntityInstance>>();
		
		while(true) {
		
			ByteBuffer byte_buffer;
			try {
				byte_buffer = engine.sendCommandBinary("get");
			} catch (IOException e) {
				throw new IfcEngineException(e);
			}
			if ( byte_buffer.limit() >= 8 ) {
				String type;
				try {
					type = engine.sendCommandText("type").toUpperCase();
				} catch (IOException e) {
					throw new IfcEngineException(e);
				}
				List<IfcOpenShellEntityInstance> entity_list;
				if ( ! instances.containsKey(type) ) {
					entity_list = new ArrayList<IfcOpenShellEntityInstance>();
					instances.put(type, entity_list);
				} else {
					entity_list = instances.get(type);
				}					
				int start_vertex = vertices_list.size();
				int start_index = indices_list.size();
				byte_buffer.order(ByteOrder.nativeOrder());
				int index = 0;
				int vcount = byte_buffer.getInt(); index += 4;
				for ( int i = 0; i < vcount*3; i ++ ) {
					vertices_list.add(byte_buffer.getFloat());
					index += 4;
				}
				int fcount = byte_buffer.getInt(); index += 4;
				for ( int i = 0; i < fcount*3; i ++ ) {
					indices_list.add(byte_buffer.getInt());
					index += 4;
				}
				if ( IfcOpenShellEngine.debug ) LOGGER.info(String.format("Vertices: %d Faces: %d", vcount,fcount));
				entity_list.add(new IfcOpenShellEntityInstance(start_vertex,start_index,fcount));
			} else {
				LOGGER.error("Invalid buffer received");
			}				
			try {
				String r = engine.sendCommandText("next");
				if ( ! "y".equals(r) ) break;
			} catch (IOException e) {
				e.printStackTrace();
				break;
			}					
		}
		
		int[] indices = new int[indices_list.size()];
		float[] vertices = new float[vertices_list.size()];
		float[] normals = new float[normals_list.size()];
				
		for(int i=0; i<indices.length; i++)
			indices[i]=indices_list.get(i).intValue();
		for(int i=0; i<vertices.length; i++)
			vertices[i]=vertices_list.get(i).floatValue();
		for(int i=0; i<normals.length; i++)
			normals[i]=normals_list.get(i).floatValue();

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

}
