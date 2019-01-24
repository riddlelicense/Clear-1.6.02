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

//
//  File: offset.cc
//
//  Description:
// 		Example implementation of a deformer. This node
//		offsets vertices according to the CV's weights.
//		The weights are set using the set editor or the
//		percent command.
//

#include <string.h>
#include <maya/MIOStream.h>
#include <math.h>

#include <maya/MPxDeformerNode.h> 
#include <maya/MItGeometry.h>
#include <maya/MPxLocatorNode.h> 

#include <maya/MFnNumericAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnMatrixData.h>

#include <maya/MFnPlugin.h>
#include <maya/MFnDependencyNode.h>

#include <maya/MTypeId.h> 
#include <maya/MPlug.h>

#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MArrayDataHandle.h>

#include <maya/MPoint.h>
#include <maya/MVector.h>
#include <maya/MMatrix.h>

#include <maya/MDagModifier.h>


class offset : public MPxDeformerNode
{
public:
						offset();
	virtual				~offset();

	static  void*		creator();
	static  MStatus		initialize();

	// deformation function
	//
    virtual MStatus      		deform(MDataBlock& 		block,
									   MItGeometry& 	iter,
									   const MMatrix& 	mat,
									   unsigned int		multiIndex);

	// when the accessory is deleted, this node will clean itself up
	//
	virtual MObject&			accessoryAttribute() const;

	// create accessory nodes when the node is created
	//
	virtual MStatus				accessoryNodeSetup(MDagModifier& cmd);

public:
	// local node attributes

	static  MObject     offsetMatrix; 	// offset center and axis
	
	static  MTypeId		id;

private:
};

MTypeId     offset::id( 0x8000c );

// local attributes
//
MObject		offset::offsetMatrix;


offset::offset() {}
offset::~offset() {}

void* offset::creator()
{
	return new offset();
}

MStatus offset::initialize()
{
	// local attribute initialization

	MFnMatrixAttribute  mAttr;
	offsetMatrix=mAttr.create( "locateMatrix", "lm");
	    mAttr.setStorable(false);
		mAttr.setConnectable(true);

	//  deformation attributes
	addAttribute( offsetMatrix);

	attributeAffects( offset::offsetMatrix, offset::outputGeom );

	return MStatus::kSuccess;
}


MStatus
offset::deform( MDataBlock& block,
				MItGeometry& iter,
				const MMatrix& /*m*/,
				unsigned int multiIndex)
//
// Method: deform
//
// Description:   Deform the point with a squash algorithm
//
// Arguments:
//   block		: the datablock of the node
//	 iter		: an iterator for the geometry to be deformed
//   m    		: matrix to transform the point into world space
//	 multiIndex : the index of the geometry that we are deforming
//
//
{
	MStatus returnStatus;
	
	// Envelope data from the base class.
	// The envelope is simply a scale factor.
	//
	MDataHandle envData = block.inputValue(envelope, &returnStatus);
	if (MS::kSuccess != returnStatus) return returnStatus;
	float env = envData.asFloat();	

	// Get the matrix which is used to define the direction and scale
	// of the offset.
	//
	MDataHandle matData = block.inputValue(offsetMatrix, &returnStatus );
	if (MS::kSuccess != returnStatus) return returnStatus;
	MMatrix omat = matData.asMatrix();
	MMatrix omatinv = omat.inverse();

	// iterate through each point in the geometry
	//
	for ( ; !iter.isDone(); iter.next()) {
		MPoint pt = iter.position();
		pt *= omatinv;
		
		float weight = weightValue(block,multiIndex,iter.index());
		
		// offset algorithm
		//
		pt.y = pt.y + env*weight;
		//
		// end of offset algorithm

		pt *= omat;
		iter.setPosition(pt);
	}
	return returnStatus;
}


/* override */
MObject&
offset::accessoryAttribute() const
//
//	Description:
//	  This method returns a the attribute to which an accessory	
//    shape is connected. If the accessory shape is deleted, the deformer
//	  node will automatically be deleted.
//
//    This method is optional.
//
{
	return offset::offsetMatrix;
}

/* override */
MStatus
offset::accessoryNodeSetup(MDagModifier& cmd)
//
//	Description:
//		This method is called when the deformer is created by the
//		"deformer" command. You can add to the cmds in the MDagModifier
//		cmd in order to hook up any additional nodes that your node needs
//		to operate.
//
//		In this example, we create a locator and attach its matrix attribute
//		to the matrix input on the offset node. The locator is used to
//		set the direction and scale of the random field.
//
//	Description:
//		This method is optional.
//
{
	MStatus result;

	// hook up the accessory node
	//
	MObject objLoc = cmd.createNode(MString("locator"),
									MObject::kNullObj,
									&result);

	if (MS::kSuccess == result) {
		MFnDependencyNode fnLoc(objLoc);
		MString attrName;
		attrName.set("matrix");
		MObject attrMat = fnLoc.attribute(attrName);

		result = cmd.connect(objLoc,attrMat,this->thisMObject(),offset::offsetMatrix);
	}
	return result;
}


// standard initialization procedures
//

MStatus initializePlugin( MObject obj )
{
	MStatus result;
	MFnPlugin plugin( obj, PLUGIN_COMPANY, "3.0", "Any");
	result = plugin.registerNode( "offset", offset::id, offset::creator, 
								  offset::initialize, MPxNode::kDeformerNode );

	return result;
}

MStatus uninitializePlugin( MObject obj)
{
	MStatus result;
	MFnPlugin plugin( obj );
	result = plugin.deregisterNode( offset::id );
	return result;
}
