//-
// ==========================================================================
// Copyright (C) 1995 - 2006 Autodesk, Inc. and/or its licensors.  All 
// rights reserved.
//
// The coded instructions, statements, computer programs, and/or related 
// material (collectively the "Data") in these files contain unpublished 
// information proprietary to Autodesk, Inc. ("Autodesk") and/or its 
// licensors, which is protected by U.S. and Canadian federal copyright 
// law and by international treaties.
//
// The Data is provided for use exclusively by You. You have the right 
// to use, modify, and incorporate this Data into other products for 
// purposes authorized by the Autodesk software license agreement, 
// without fee.
//
// The copyright notices in the Software and this entire statement, 
// including the above license grant, this restriction and the 
// following disclaimer, must be included in all copies of the 
// Software, in whole or in part, and all derivative works of 
// the Software, unless such copies or derivative works are solely 
// in the form of machine-executable object code generated by a 
// source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND. 
// AUTODESK DOES NOT MAKE AND HEREBY DISCLAIMS ANY EXPRESS OR IMPLIED 
// WARRANTIES INCLUDING, BUT NOT LIMITED TO, THE WARRANTIES OF 
// NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR 
// PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE, OR 
// TRADE PRACTICE. IN NO EVENT WILL AUTODESK AND/OR ITS LICENSORS 
// BE LIABLE FOR ANY LOST REVENUES, DATA, OR PROFITS, OR SPECIAL, 
// DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES, EVEN IF AUTODESK 
// AND/OR ITS LICENSORS HAS BEEN ADVISED OF THE POSSIBILITY 
// OR PROBABILITY OF SUCH DAMAGES.
//
// ==========================================================================
//+

/* 

	file : fileIOMsgCmd.cpp
	class: PreLoad
	----------------------
	This is an example to demonstrate the usages of :

	MString MFileIO::beforeImportFile(MStatus*); 
	MString MFileIO::beforeOpenFile(MStatus*);
	MString MFileIO::beforeExportFile(MStatus*);
	MString MFileIO::beforeReferenceFile(MStatus*);

	Return value of MS::kSuccess indicates correct value returns
		
*/

#include <maya/MFnPlugin.h>
#include <maya/MSceneMessage.h>
#include <maya/MStatus.h>
#include <maya/MPxCommand.h>
#include <maya/MMessage.h>
#include <maya/MGlobal.h>
#include <maya/MFileIO.h>
#include <maya/MCallbackIdArray.h>


class PreLoad : public MPxCommand
{

	public:
		// callback functions.
		static void preOpenFunc(void* clientData);
		static void preImportFunc(void* clientData);
		static void preSaveFunc(void* clientData);
		static void preExportFunc(void* clientData);
		static void preReferenceFunc(void* clientData);

		virtual MStatus doIt( const MArgList& );		
		static void *creator();
		static MCallbackIdArray& getCallbackIds();

	private:
		//callback ids .. used later for removal
		static MCallbackIdArray ids;	

};



MCallbackIdArray PreLoad::ids;


// This is the call back function at Pre Import stage.
void PreLoad::preOpenFunc(void* clientData)
{
	MString msg = "FILE TO BE OPENED IS ";
	MStatus status = MS::kSuccess;

	MString file = MFileIO::beforeOpenFilename(&status);
		
	// check the status and then append accordingly
	msg +=  ( status == MS::kSuccess) ? file : "ERROR: Could not be retrieved" ;

	MGlobal::displayInfo(msg);
	
}

// This is the call back function at Pre Import stage.
void PreLoad::preImportFunc(void* clientData)
{
	MString msg = "PRE IMPORT FILE IS ";
	MStatus status = MS::kSuccess;

	MString file = MFileIO::beforeImportFilename(&status);

	// check the status and then append accordingly
	msg +=  ( status == MS::kSuccess) ? file : "ERROR: Could not be retrieved" ;
	
	MGlobal::displayInfo(msg);
}


// This is the call back function at Pre Save stage.
void PreLoad::preSaveFunc(void* clientData)
{
	MString msg = "FILE TO BE SAVED IS";
	MStatus status = MS::kSuccess;

	MString file = MFileIO::beforeSaveFilename(&status);

	//check the status and then append accordingly
	msg +=  ( status == MS::kSuccess) ? file : "ERROR: File name could not be retrieved" ;
	
	MGlobal::displayInfo(msg);
}


// This is the call back function at Pre Export stage.
void PreLoad::preExportFunc(void* clientData)
{
	MString msg = "FILE TO BE EXPORTED IS";
	MStatus status = MS::kSuccess;

	MString file = MFileIO::beforeSaveFilename(&status);

	 // check the status and then append accordingly
	msg +=  ( status == MS::kSuccess) ? file : "ERROR: File name could not be retrieved" ;
	
	MGlobal::displayInfo(msg);
}


// This is the call back function at Pre Reference stage.
void PreLoad::preReferenceFunc(void* clientData)
{
	MString msg = "FILE TO BE REFERENCED IS";
	MStatus status = MS::kSuccess;

	MString file = MFileIO::beforeReferenceFilename(&status);

	//check the status and then append accordingly
	msg +=  ( status == MS::kSuccess) ? file : "ERROR: File name could not be retrieved" ;
	
	MGlobal::displayInfo(msg);
}

MStatus PreLoad::doIt( const MArgList& )
{
	MStatus status = MS::kSuccess;
	MGlobal::displayInfo("PLUGIN LOADED");

	// add the function call backs
	// and store call back ids for removal later
	ids.append(MSceneMessage::addCallback ( MSceneMessage::kBeforeOpen, preOpenFunc )) ;
	ids.append(MSceneMessage::addCallback ( MSceneMessage::kBeforeImport, preImportFunc )) ;
	ids.append(MSceneMessage::addCallback ( MSceneMessage::kBeforeSave, preSaveFunc )) ;
	ids.append(MSceneMessage::addCallback ( MSceneMessage::kBeforeExport, preExportFunc )) ;
	ids.append(MSceneMessage::addCallback ( MSceneMessage::kBeforeReference, preReferenceFunc )) ;
	
	return status ; 

}	



void* PreLoad::creator() 
{	
	return new PreLoad;
}


MCallbackIdArray& PreLoad::getCallbackIds()
{
	return ids;
}



// standard initialize and uninitialize functions


MStatus initializePlugin(MObject obj)
{
	MFnPlugin pluginFn(obj, PLUGIN_COMPANY, "6.0");

	MStatus status;
	status = pluginFn.registerCommand("fileIOMsgCmd", PreLoad::creator);

	if( !status)
		status.perror("register Command failed");

	return status;
}


MStatus uninitializePlugin ( MObject obj )
{
	MFnPlugin pluginFn(obj);
	MStatus status = MS::kSuccess;
	
	//remove call backs
	MMessage::removeCallbacks(PreLoad::getCallbackIds());
	status = pluginFn.deregisterCommand( "fileIOMsgCmd");
	
	return status;
}

