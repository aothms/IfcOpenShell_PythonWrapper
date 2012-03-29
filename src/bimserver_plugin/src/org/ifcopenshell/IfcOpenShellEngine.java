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
* An intermediate between the Plugin implementation and the Model              *
* implementation, nothing fancy going on here.                                 *
*                                                                              *
********************************************************************************/

package org.ifcopenshell;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;

import org.bimserver.plugins.ifcengine.IfcEngine;
import org.bimserver.plugins.ifcengine.IfcEngineException;
import org.bimserver.plugins.ifcengine.IfcEngineModel;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class IfcOpenShellEngine implements IfcEngine {
	private static final Logger LOGGER = LoggerFactory.getLogger(IfcOpenShellEngine.class);
	public static final Boolean debug = false;
	private String filename;

	public IfcOpenShellEngine(String fn) throws IOException {
		filename = fn;
	}

	@Override
	public void init() throws IfcEngineException {
		LOGGER.info("Initializing IfcOpenShell engine");
	}
	
	@Override
	public void close() {
		LOGGER.info("Closing IfcOpenShell engine");
	}

	@Override
	public IfcEngineModel openModel(File ifcFile) throws IfcEngineException {
		try {
			return openModel(new FileInputStream(ifcFile),(int)ifcFile.length());
		} catch (IOException e) {
			throw new IfcEngineException("Failed to open model");		
		}
	}

	@Override
	public IfcEngineModel openModel(InputStream inputStream, int size) throws IfcEngineException {
		try {
			byte[] bytes = new byte[size];
			inputStream.read(bytes);
			return openModel(bytes);
		} catch (IOException e) {
			throw new IfcEngineException("Failed to open model");
		}
	}

	@Override
	public IfcEngineModel openModel(byte[] bytes) throws IfcEngineException {
		return new IfcOpenShellModel(filename,bytes);
	}
}