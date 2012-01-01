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
* This class ensures that a valid binary is available for the platform the     *
* code is running on.                                                          *
*                                                                              *
********************************************************************************/

package org.ifcopenshell;

import java.io.File;
import java.io.IOException;
import java.net.URL;

import org.bimserver.plugins.PluginContext;
import org.bimserver.plugins.PluginManager;
import org.bimserver.plugins.ifcengine.IfcEngine;
import org.bimserver.plugins.ifcengine.IfcEngineException;
import org.bimserver.plugins.ifcengine.IfcEnginePlugin;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class IfcOpenShellEnginePlugin implements IfcEnginePlugin {
	private static final Logger LOGGER = LoggerFactory.getLogger(IfcOpenShellEnginePlugin.class);
	private boolean initialized = false;
	private String filename;

	@Override
	public IfcEngine createIfcEngine() throws IfcEngineException {
		try {
			return new IfcOpenShellEngine(filename);
		} catch (IOException e) {
			throw new IfcEngineException(e);
		}
	}

	@Override
	public String getDescription() {
		return "Open source IFC geometry engine<br>visit <a href='http://ifcopenshell.org'>ifcopenshell.org</a>";
	}

	@Override
	public String getVersion() {
		return "0.3.0RC2";
	}

	@Override
	public void init(PluginManager pluginManager) {
		PluginContext pluginContext = pluginManager.getPluginContext(this);
		String os = System.getProperty("os.name").toLowerCase();
		String libraryName = String.format("lib/%s/%sIfcJni.%s",
				System.getProperty("sun.arch.data.model"),
				os.contains("windows") ? "" : "lib",
				os.contains("windows") ? "dll" : (os.contains("linux") ? "so" : "dylib" )
		);
		URL u = pluginContext.getResourceAsUrl(libraryName);
		if ( u == null ) {
			LOGGER.error("IfcOpenShell is not available for your platform");
		} else {
			filename = u.getPath().toString();
			initialized = new File(filename).exists();
		}
	}

	@Override
	public boolean isInitialized() {
		return initialized;
	}

}
