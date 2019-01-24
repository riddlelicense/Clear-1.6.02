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
 
#include <math.h>
#include <maya/MIOStream.h>
#include <maya/MPxCommand.h>
#include <maya/MStatus.h>
#include <maya/MObject.h>
#include <maya/MFnPlugin.h>
#include <maya/MString.h>
#include <maya/MArgList.h>
#include <maya/MGlobal.h>
#include <maya/MPoint.h>
#include <maya/MVector.h>
#include <maya/MMatrix.h>
#include <maya/MDagPath.h>

#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MItSurfaceCV.h>
#include <maya/MItMeshVertex.h>

//////////////////////
// Class Definition //
//////////////////////

class surfaceTwist : public MPxCommand {
public:

					surfaceTwist();
	virtual			~surfaceTwist();

	virtual MStatus	doIt( const MArgList& args );

	static void*	creator();
};

//////////////////////////
// Class Implementation //
//////////////////////////

#define NUM_SPANS        30
#define WIDTH            10.0
#define VERTICAL_SCALING 4.0

surfaceTwist::~surfaceTwist() {}

surfaceTwist::surfaceTwist() {}

void* surfaceTwist::creator()
{
	return new surfaceTwist;
}

static MStatus twistNurbsSurface(MDagPath& objectPath, MObject& component)
{
	MStatus status;

	MPoint  center;
	MVector toCenter( -center.x, 0.0, -center.y );
	double  rotFactor = 0.5;

	// We have a nurbs surface or component
	//
	MItSurfaceCV cvIter( objectPath, component, true, &status );

	if ( MS::kSuccess == status ) {
		// We successfully created a nurbs surface iterator
		//
		for ( ; !cvIter.isDone(); cvIter.nextRow() ) {
			for ( ; !cvIter.isRowDone(); cvIter.next() ) {
				// Get the location of the CV
				//
				MPoint pnt = cvIter.position( MSpace::kWorld );
				pnt = pnt + toCenter;
				// Calculate rotation in radians about the y-axis
				//
				double rotation = pnt.y * rotFactor;
				MMatrix rotMatrix;
				// Set matrix to a rotation about the y axis
				//
				rotMatrix(0,0) = cos( rotation );
				rotMatrix(0,2) = sin( rotation );
				rotMatrix(2,0) = -sin( rotation );
				rotMatrix(2,2) = cos( rotation );
				pnt = ( pnt * rotMatrix ) - toCenter;

				status = cvIter.setPosition( pnt, MSpace::kWorld );
				if ( MS::kSuccess != status )
					break;
			}
		}
		// Tell maya to redraw the surface with all of our changes
		//
		cvIter.updateSurface();
		return MS::kSuccess;
	} else
		return MS::kFailure;
}

static MStatus twistPolygon(MDagPath& objectPath, MObject& component)
{
	MStatus status;

	MPoint  center;
	MVector toCenter( -center.x, 0.0, -center.y );
	double  rotFactor = 0.5;

	MItMeshVertex vertIter( objectPath, component, &status );

	if ( MS::kSuccess == status ) {
		// We successfully created a polygon vertex iterator
		//

		for ( ; !vertIter.isDone(); vertIter.next() ) {
			// Get the location of the vertex
			//
			MPoint pnt = vertIter.position( MSpace::kWorld );
			pnt = pnt + toCenter;
			// Calculate rotation in radians about the y-axis
			//
			double rotation = pnt.y * rotFactor;
			MMatrix rotMatrix;
			// Set matrix to a rotation about the y axis
			//
			rotMatrix(0,0) = cos( rotation );
			rotMatrix(0,2) = sin( rotation );
			rotMatrix(2,0) = -sin( rotation );
			rotMatrix(2,2) = cos( rotation );
			pnt = ( pnt * rotMatrix ) - toCenter;

			status = vertIter.setPosition( pnt, MSpace::kWorld );
			if ( MS::kSuccess != status )
				break;
		}
		// Tell maya to redraw the surface with all of our changes
		//
		vertIter.updateSurface();
		return MS::kSuccess;
	} else
		return MS::kFailure;
}

MStatus surfaceTwist::doIt( const MArgList& )
//
//	Description:
//		Plugin command to test Selection List Iterator.
//
//
{
	MStatus status;

	// Create an iterator for the active selection list
	//
	MSelectionList slist;
	MGlobal::getActiveSelectionList( slist );
	MItSelectionList iter( slist );

	if (iter.isDone()) {
		cerr << "Nothing selected\n";
		return MS::kFailure;
	}

	MDagPath objectPath;
	MObject component;

	for ( ; !iter.isDone(); iter.next() ) {
		status = iter.getDagPath( objectPath, component );

		if (objectPath.hasFn(MFn::kNurbsSurface))
			status = twistNurbsSurface(objectPath, component);
		else if (objectPath.hasFn(MFn::kMesh))
			status = twistPolygon(objectPath, component);
		else {
			cerr << "Selected object is not a NURBS surface or a polygon\n";
			return MS::kFailure;
		}
	}
	return status;
}

//////////////////////////////////
// Register command with system //
//////////////////////////////////


MStatus initializePlugin( MObject obj )
{
	MStatus   status;
	MFnPlugin plugin( obj, PLUGIN_COMPANY, "3.0", "Any");

	status = plugin.registerCommand( "surfaceTwist", surfaceTwist::creator );
	if (!status) {
		status.perror("registerCommand");
		return status;
	}

	return status;
}

MStatus uninitializePlugin( MObject obj )
{
	MStatus   status;
	MFnPlugin plugin( obj );

	status = plugin.deregisterCommand( "surfaceTwist" );
	if (!status) {
		status.perror("deregisterCommand");
		return status;
	}

	return status;
}
