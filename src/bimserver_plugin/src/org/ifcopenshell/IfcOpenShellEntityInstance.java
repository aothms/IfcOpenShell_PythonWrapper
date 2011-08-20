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

import org.bimserver.plugins.ifcengine.IfcEngineException;
import org.bimserver.plugins.ifcengine.IfcEngineInstance;
import org.bimserver.plugins.ifcengine.IfcEngineInstanceVisualisationProperties;

public class IfcOpenShellEntityInstance implements IfcEngineInstance {
	private int startVertex;
	private int startIndex;
	private int primitiveCount;

	public IfcOpenShellEntityInstance(int startVertex, int startIndex, int primitiveCount) {
		this.startVertex = startVertex;
		this.startIndex = startIndex;
		this.primitiveCount = primitiveCount;		
	}
	@Override
	public IfcEngineInstanceVisualisationProperties getVisualisationProperties()
			throws IfcEngineException {
		return new IfcEngineInstanceVisualisationProperties(startVertex, startIndex, primitiveCount);
	}

}
