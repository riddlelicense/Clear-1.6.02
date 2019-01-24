//-
// ==========================================================================
// Copyright 1995,2006,2008 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk
// license agreement provided at the time of installation or download,
// or which otherwise accompanies this software in either electronic
// or hard copy form.
// ==========================================================================
//+

// Example Plugin: instancerListCmd.cpp
//
//	This plugin demonstrates how to use the MFnInstancer class and the 
//	MItInstancer class to enumerate particle instances in a scene.  If
//	one or more instancer nodes are selected when the command is invoked,
//	MFnInstancer will be used to enumerate each instancer's instances, with
//	both the instancesForParticle() and allInstances() methods exercised.
//	If no instancers are selected, then MItInstancer is used to iterate through
//	all the particle instances in the scene.
//

#include <maya/MSimple.h>
#include <maya/MSelectionList.h>
#include <maya/MGlobal.h>
#include <maya/MItSelectionList.h>
#include <maya/MString.h>
#include <maya/MFnInstancer.h>
#include <maya/MItInstancer.h>
#include <maya/MDagPathArray.h>
#include <maya/MPoint.h>
#include <maya/MMatrixArray.h>

DeclareSimpleCommand( listParticleInstances, PLUGIN_COMPANY, "8.0");

void printInstancerUsingFunctionSet( const MDagPath& instancerPath )
{
	char str[256];
	MString pathName = instancerPath.fullPathName();
	sprintf( str, "Instancer %s:", pathName.asChar() );
	MGlobal::displayInfo( MString(str) );
		
	MFnInstancer fnInst( instancerPath );
	int numParticles = fnInst.particleCount();
	sprintf( str, "    num particles = %d", numParticles );
	
	//
	//	Step 1: Use the MFnInstancer::instancesForParticle() to enumerate
	//			the paths instanced under each particle.  
	//
	MGlobal::displayInfo( "    Using instancesForParticle()...." );
	MMatrix instancerWorldMatrix = instancerPath.inclusiveMatrix();
	int p = 0;
	for( p = 0; p < numParticles; p++ )
	{
		MMatrix particleMatrix;
		MDagPathArray particlePaths;
		int numInstances = fnInst.instancesForParticle( p, particlePaths, particleMatrix );

		//	iterate over all the instances under this particle
		//		
		for( int i = 0; i < numInstances; i++ )
		{
			const MDagPath& instancedPath = particlePaths[i];
			MMatrix instancedPathMatrix = instancedPath.inclusiveMatrix();

			//	the final world position of the instanced shape is determined
			//	by the original path's matrix combined with the offset matrix
			//	provided by the instancer
			//
			MMatrix finalMatrixForPath = instancedPathMatrix * particleMatrix;
			MPoint finalPoint = MPoint::origin * finalMatrixForPath;
				
			MString instancedPathName = instancedPath.fullPathName();
			sprintf( str, "        Path %-50s at position (%lf,%lf,%lf)", instancedPathName.asChar(), finalPoint.x, finalPoint.y, finalPoint.z );
			MGlobal::displayInfo( str );
		}
	}
	
	//
	//	Step 2: Use the MFnInstancer::allInstances() method to enumerate all
	//			particle instances generated by this instancer.  The same
	//			information that was extracted one particle at a time in 
	//			Step 1 is now retrieved with one function call, and stored in
	//			a set of arrays.
	//
	MGlobal::displayInfo( "    Using allInstances()...." );
	MDagPathArray allPaths;
	MMatrixArray allMatrices;
	MIntArray pathIndices;
	MIntArray pathStartIndices;
	
	fnInst.allInstances( allPaths, allMatrices, pathStartIndices, pathIndices );
	for( p = 0; p < numParticles; p++ )
	{
		MMatrix particleMatrix = allMatrices[p];

		//	the number of paths instanced under a particle is computed by
		//	taking the difference between the starting path index for this
		//	particle and that of the next particle.  The size of the start
		//	index array is always one larger than the number of particles.
		//
		int numPaths = pathStartIndices[p+1]-pathStartIndices[p];
		
		//	the values pathIndices[pathStart...pathStart+numPaths] give the
		//	indices in the allPaths array of the paths instanced under this
		//	particular particle.  Remember, different paths can be instanced
		//	under each particle.
		//
		int pathStart = pathStartIndices[p];

		//	loop through the instanced paths for this particle
		//
		for( int i = pathStart; i < pathStart+numPaths; i++ )
		{
			int curPathIndex = pathIndices[i];
			const MDagPath& curPath = allPaths[curPathIndex];

			MMatrix instancedPathMatrix = curPath.inclusiveMatrix();
			MMatrix finalMatrixForPath = instancedPathMatrix * particleMatrix;
			MPoint finalPoint = MPoint::origin * finalMatrixForPath;
				
			MString instancedPathName = curPath.fullPathName();
			sprintf( str, "        Path %-50s at position (%lf,%lf,%lf)", instancedPathName.asChar(), finalPoint.x, finalPoint.y, finalPoint.z );
			MGlobal::displayInfo( str );
		}
	}
}

void printAllInstancesUsingIterator()
{
	//
	//	Just use the MItInstancer iterator to enumerate all particles in 
	//	all instancers in the scene.
	//
	MItInstancer it;
	while( !it.isDone() )
	{
		MObject instancerNode = it.instancer();
		MDagPath instancerPath = it.instancerPath();
		MDagPath instancePath = it.path();
		MMatrix instanceMatrix = it.matrix();

		MString instancerNodeName = MFnDependencyNode(instancerNode).name();
		MString instancerPathName = instancerPath.fullPathName();
		MString instancePathName = instancePath.fullPathName();
		
		MMatrix pathMatrix = instancePath.inclusiveMatrix();
		MMatrix finalMatrixForPath = pathMatrix * instanceMatrix;
		MPoint pos = MPoint::origin * finalMatrixForPath;

		char str[512];
		sprintf( str, "Instancer node %s, instancer path %s, instancing path %s at position (%lf,%lf,%lf)",
				instancerNodeName.asChar(), instancerPathName.asChar(), instancePathName.asChar(), pos.x, pos.y, pos.z );
		MGlobal::displayInfo( MString(str) );
		it.next();
	}
}

MStatus listParticleInstances::doIt( const MArgList& )
{
	MSelectionList curSel;
	MGlobal::getActiveSelectionList( curSel );
		
	MItSelectionList it( curSel, MFn::kInstancer );	
		
	if( it.isDone() )
	{
		//	use MItInstancer to enumerate all particle instances 
		//
		MGlobal::displayInfo( "Using iterator to enumerate all particle instances..." );
		printAllInstancesUsingIterator();
	}
	else
	{
		//	use MFnInstancer to list instances
		//
		while( !it.isDone() )
		{
			MDagPath instancerPath;
			if( it.getDagPath( instancerPath ) == MS::kSuccess )
			{
				printInstancerUsingFunctionSet( instancerPath );
			}
			else
			{
				MGlobal::displayError( "Error retrieving instancer" );
				return MS::kFailure;
			}
			it.next();
		}
	}
	return MS::kSuccess;
}
	
	
