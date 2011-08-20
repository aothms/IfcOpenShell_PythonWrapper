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

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashSet;
import java.util.Set;

import org.apache.commons.io.FileUtils;
import org.apache.commons.io.IOUtils;
import org.bimserver.plugins.Plugin;
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
	private File nativeFolder;
	private File executableFile;

	@Override
	public IfcEngine createIfcEngine() throws IfcEngineException {
		try {
			return new IfcOpenShellEngine(executableFile);
		} catch (IOException e) {
			throw new IfcEngineException(e);
		}
	}

	@Override
	public String getDescription() {
		return "Open source IFC geometry engine<br>visit <a href='http://ifcopenshell.org'>ifcopenshell.org</a>";
	}

	@Override
	public String getName() {
		return "IfcOpenShell";
	}

	@Override
	public Set<Class<? extends Plugin>> getRequiredPlugins() {
		return new HashSet<Class<? extends Plugin>>();
	}

	@Override
	public String getVersion() {
		return "0.2.0";
	}

	@Override
	public void init(PluginManager pluginManager) {
		PluginContext pluginContext = pluginManager.getPluginContext(this);
		String os = System.getProperty("os.name").toLowerCase();
		String libraryName = "IfcStdin";
		if (os.contains("windows")) libraryName += ".exe";
		InputStream inputStream = pluginContext.getResourceAsInputStream("lib/" + System.getProperty("sun.arch.data.model") + "/" + libraryName);
		if (inputStream != null) {
			File tmpFolder = new File(pluginManager.getHomeDir().getAbsolutePath(), "tmp");
			nativeFolder = new File(tmpFolder, "IfcOpenShell");
			try {
				if (nativeFolder.exists()) {
					FileUtils.deleteDirectory(nativeFolder);
				}
				nativeFolder.mkdir();
				executableFile = new File(nativeFolder, libraryName);
				LOGGER.info(String.format("Creating file %s", executableFile.getAbsolutePath()));
				IOUtils.copy(inputStream, new FileOutputStream(executableFile));
				executableFile.setExecutable(true);
				initialized = true;
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}

	@Override
	public boolean isInitialized() {
		return initialized;
	}

}
