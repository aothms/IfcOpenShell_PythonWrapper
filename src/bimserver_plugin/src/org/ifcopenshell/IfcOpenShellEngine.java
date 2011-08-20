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

import java.io.BufferedWriter;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.nio.ByteBuffer;

import org.bimserver.plugins.ifcengine.IfcEngine;
import org.bimserver.plugins.ifcengine.IfcEngineException;
import org.bimserver.plugins.ifcengine.IfcEngineModel;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class IfcOpenShellEngine implements IfcEngine {
	private static final Logger LOGGER = LoggerFactory.getLogger(IfcOpenShellEngine.class);
	private InputStream stdout_stream, stderr_stream;
	private OutputStream stdin_stream;
	private BufferedWriter stdin;
	public static final Boolean debug = true;
	public static final int timeout = 20000;
	
	private byte[] waitForData() throws IOException {
		try {
			int t = 0;
			while ( true ) {
				int av = stdout_stream.available();
				if ( av == 0 ) {
					if ( ++t > timeout ) 
						throw new IOException();
					Thread.sleep(1);
					continue;
				}
				byte[] bytes = new byte[av];
				stdout_stream.read(bytes);
				return bytes;
			}
		} catch (Throwable e) { 
			throw new IOException(); 
		}
	}
	
	private void printErrors() throws IOException {
		int av = stderr_stream.available();
		if ( av > 0 ) {
			LOGGER.error("Errors were encountered while processing Ifc files");
			byte[] bytes = new byte[av];
			stdout_stream.read(bytes);
			String errors = new String(bytes);
			System.out.println(errors);
		}
	}
	
	public String waitForResponseString() throws IOException {
		return new String(waitForData());
	}	
	private ByteBuffer waitForResponseBuffer() throws IOException {
		return ByteBuffer.wrap(waitForData());
	}
	
	public String sendCommandText(String command) throws IOException {
		if ( debug ) LOGGER.info(String.format(">>> %s", command));
		stdin.write(String.format("%s\n", command));
		stdin.flush();	
		String response = waitForResponseString();
		if ( debug ) LOGGER.info(String.format("<<< %s", response));
		return response;
	}
	
	public ByteBuffer sendCommandBinary(String command) throws IOException {
		if ( debug ) LOGGER.info(String.format(">>> %s", command));
		stdin.write(String.format("%s\n", command));
		stdin.flush();
		if ( debug ) LOGGER.info("<<< [binary data]");
		return waitForResponseBuffer();
	}
	
	public IfcOpenShellEngine(File exec) throws IOException {
		LOGGER.info("Creating IfcOpenShell engine");
		if ( true ) {
			Runtime run = Runtime.getRuntime();
			Process pr = run.exec(exec.getAbsolutePath());			
			stdin_stream = pr.getOutputStream();
			stdin = new BufferedWriter(new OutputStreamWriter(stdin_stream));
			stderr_stream = pr.getErrorStream();
			stdout_stream = pr.getInputStream();
			String response = waitForResponseString();
			if ( debug ) LOGGER.info(String.format("<<< %s",response));
		}
	}

	@Override
	public void close() {
		LOGGER.info("Closing IfcOpenShell engine");
		try {
			printErrors();
			sendCommandText("exit");
		} catch (IOException e) {
			e.printStackTrace();
		}		
	}

	@Override
	public IfcEngineModel openModel(File ifcFile) throws IfcEngineException {
		return new IfcOpenShellModel(ifcFile, this);
	}

	@Override
	public IfcEngineModel openModel(InputStream inputStream, int size) throws IfcEngineException {
		try {
			byte[] bytes = new byte[size];
			inputStream.read(bytes);
			return openModel(bytes);
		} catch (Throwable e1) {
			throw new IfcEngineException("Failed to open model");
		}
	}

	@Override
	public IfcEngineModel openModel(byte[] bytes) throws IfcEngineException {
		try {
			sendCommandText("loadstdin");
			sendCommandText(String.format("%d",bytes.length));
			stdin_stream.write(bytes);
			stdin_stream.flush();
			String response = waitForResponseString();
			if ( debug ) LOGGER.info(String.format("<<< %s", response));
			return new IfcOpenShellModel(this);
		} catch (Throwable e1) {
			throw new IfcEngineException("Failed to open model");
		}		
	}

}
