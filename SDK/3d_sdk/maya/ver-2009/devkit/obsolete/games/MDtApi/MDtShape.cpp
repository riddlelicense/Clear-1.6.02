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

//@Include: MDt.h 
//@{
//@Man:  {\large {\bf Description}}
/**

The functions in MDtShape.cpp provide access to polygonal meshes in reference 
to Shapes

{\bf Mesh Construction} \\
Shapes are transformation and geometry (polygonal or Nurbs) that are contained
in the Maya scene file.  For each shape there is a structure of information 
that is kept internally to be used as a shortcut to the required data to be
output.
The following lists are used by the create method to construct a mesh:

    \begin{itemize}
    \item {\bf vertex list}
    All of the vertices for the object are stored in a master vertex list which
    is an array indexed from 0 to vertexCount. Vertices in this list can be
    shared among polygons.

	\item {\bf normal list}
	All of the normals for the object. indexed from 0 to normalCount.

    \item {\bf UV list}
    All of the uv texture coordinates for the object. indexed from 0 to 
    stuvCount.

    \item {\bf polygon normal list}
    All of the polygon normals for the object. indexed from 0 to normalPCount.

    \item {\bf average vertex color list}
    All of the average vertex colors for the object. indexed from 0 to 
    vertexCount.

    \item {\bf vertex color list}
    All of the vertex colors  for the object. indexed from 0 to vfColorCount.

    \item {\bf face Indices list}
    A list of what vertices belong to each face.  Faces are separated with an
    index of -1.  There is count of elements in the list structure.

    \item {\bf normal Indices list}
    A list of what normals belong to each face.  Faces are separated with an
    index of -1.  There is count of elements in the list structure.

    \item {\bf texture Indices list}
    A list of what texture vertices belong to each face.  Faces are separated 
	with an index of -1.  There is count of elements in the list structure.

    \item {\bf polygon normals Indices list}
    A list of what polygon normals belong to each face.  Faces are separated 
    with an index of -1.  There is count of elements in the list structure.

    \item {\bf vertex colors Indices list}
    A list of what vertex colors belong to each face.  Faces are separated 
    with an index of -1.  There is count of elements in the list structure.

    \item {\bf Group list}
    A list of what groups are associated with this shape

	\item {\bf temporary Maya Object references}
	MObject and MDagPath references to the orignal Maya object (polygonal or 
    Nurb).

    \end{itemize}

*/
// *****************************************************************************


#include <stdlib.h>
#include <stdio.h>				
#include <string.h>
#include <math.h>

// The Maya API header files.
//

#include <maya/MIOStream.h>
#include <maya/MItDag.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshVertex.h>
#include <maya/MItSelectionList.h>
#include <maya/MPoint.h>
#include <maya/MFnMesh.h>
#include <maya/MFnMeshData.h>
#include <maya/MFnNurbsSurface.h>
#include <maya/MTesselationParams.h>
#include <maya/MGlobal.h>
#include <maya/MFnTransform.h>
#include <maya/MMatrix.h>
#include <maya/MDagPath.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlugArray.h>
#include <maya/MPlug.h>
#include <maya/MFnSet.h>
#include <maya/MSelectionList.h>
#include <maya/MDagPathArray.h>
#include <maya/MObjectArray.h>
#include <maya/MFnIkJoint.h>

#include <maya/MFloatPointArray.h>
#include <maya/MFloatPoint.h>
#include <maya/MFloatArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MFloatVector.h>

#include <maya/MAnimControl.h>
#include <maya/MTime.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MItDependencyGraph.h>


#ifndef MAYA101
#include <maya/MComputation.h>
#endif


// The DSO I/O module public header files.
//

#include <MDt.h>	 
#include <MDtExt.h>

#include <MDtLight.h>
#include <MDtCamera.h>

#ifdef WIN32
#pragma warning(disable: 4244)
#endif // WIN32

enum{ kFailure, kSuccess };

// The defines, macros, and magic numbers.
//
#define DT_BLOCK_SIZE	   16
#define NO_GROUP	       -1
#define ALLOC_SIZE		   500

// bit mask values for the IsValid() functions
// 
#define DT_SCENE_VALID_MASK 	0x01000000
#define DT_CAMERA_VALID_MASK  	0x02000000
#define DT_LIGHT_VALID_MASK 	0x04000000
#define DT_SHAPE_VALID_MASK 	0x08000000
#define DT_MATERIAL_VALID_MASK  0x10000000
#define DT_VALID_BIT_MASK 		0x00ffffff

// The group table item structure.
//
typedef struct GroupStruct
{
    int			valid_bits;
	GroupStruct	* parentStruct;
	MObject	    transformNode;
	MObject	    shader;
	MObject 	surfaceNode;
    char		* mtlName;
	int			textureCount;
	MObject     textures;
	char	    * textureName;
} GroupStruct;

typedef struct
{
    int  	   	count;             
    int        *list;              
} FaceListStruct;

// The shape table item structure:
//
// Note:
//              --------------
//              | parentNode |
//              --------------
//                    |
//              -----------------
//              | transformNode |
//              -----------------
//                    |
//              -------------
//              | shapeNode |
//              -------------
//
typedef struct ShapeStruct
{
    int				valid_bits;		// valid bits for the OBJ shape, not used
    int             instance;		// shapeId of the original shape.
	ShapeStruct		*firstParentStruct;		
	MDagPath		*shapeDagPath;
	MDagPath		*parentDagPath;
    MObject 		transformNode;  
	MObject 	    surfaceNode;    // Original surface shape node before tesselation.
	                                // mesh shape node or NURBS surface node.
	MObject			shapeNodeTess;	// new tesselated shape node
	MObject 		shapeNode;      // mesh shape node, orginal or tesselated.
    int				pathType;		// 1 transform, 2 mesh
	int				groupCount;		// Count of material groups in this shape.
    GroupStruct *	groupStructs;	// Array of material groups structures.
    FaceListStruct  *faceIdx;       // Array of lists:
	                                // Each group has a face list.
	                                // For each list, -1 is the separator for each face.
    FaceListStruct  *stuvIdx;		// -1 is the separator for each face.
    FaceListStruct  *normalIdx;		// -1 is the separator for each face.
    FaceListStruct  *normPIdx;		// -1 is the separator for each face.
	FaceListStruct	*usedGroups; 	// -1: shader not used. 
	FaceListStruct	*vfColorIdx;	// -1 is the separator for each face.

	                                // 0: first shader to the groupStructs.
	DtVec3f			*vertexList;    // List of vertices in one order.
	DtVec3f			*normalList;	// List of normals in another order.
	DtVec2f			*stuvList;		// List of uv cooridinates in another order.
	DtVec3f			*normalPList;	// List of polygon normals in another order.
	DtRGBA   		*vertexColor;	// List of vertex colors in another order.
	DtFltRGBA		*vfColorList;	// List of vertex Face colors
	int				vertexCount;	
	int				stuvCount;
    int             normalCount;
	int				normalPCount;
	int				vfColorCount;
	int				texCount;		// Count of textures in this shape.

	MIntArray		*vtxAnimKeyFrames;
	MIntArray		*trsAnimKeyFrames;
	bool			vtxAnimCheck;
	bool			trsAnimCheck;

} ShapeStruct;


// The shape object instance data structure.
//
typedef struct
{
    int				shapeCount;		// Count of shapes.
    ShapeStruct	    *shapes;		// Array of ShapeStructs
	int				texCount;		// Texture count in scene.
} DtPrivate;


// Private data.
//
static DtPrivate	 *local 	    = NULL;

// static AlDagNode 	 *topNode		= NULL;

int **PolygonIndexTable = NULL;
int **UVIndexTable      = NULL;
int **NormalIndexTable  = NULL;

// private function prototypes
//
int processDagNode	( MDagPath & transformPath,
					  MObject transformNode, 
					  MObject parentNode );
int addTransformMesh( MDagPath &transformPath,
					  MObject transformNode,
					  MObject surfaceNode, 
					  MObject shapeNode,
					  MObject parentNode );
int processMesh 	( MObject transformNode,
                      MObject surfaceNode,
					  MObject currentNode );
int processSurface	( MDagPath &transformPath,
					  MObject transformNode, 
					  MObject surfaceNode, 
					  MObject shapeNode,
					  MObject parentNode );

void DtExt_ShapeDelete();
int traverseDag( void );

/////////////////////////////
// Global helper functions //
/////////////////////////////
    
bool isObjectVisible( const MDagPath & path )
//   
//  Description:
//      Check if the given object is visible
//  
{
    MFnDagNode fnDN( path );
    // Check the visibility attribute of the node
    //
    MPlug vPlug = fnDN.findPlug( "visibility" );
    // Also check to see if the node is an intermediate object in
    // a computation.  For example, it could be in the middle of a 
    // chain of deformations.  Intermediate objects are not visible.
    //
    MPlug iPlug = fnDN.findPlug( "intermediateObject" );
    
    bool visible, intermediate;
    vPlug.getValue( visible );
    iPlug.getValue( intermediate );
    
    return  visible && !intermediate;
}   

bool areObjectAndParentsVisible( const MDagPath & path )
//   
//  Description:
//      Check if this object and all of its parents are visible.  In Maya,
//      visibility is determined by  heirarchy.  So, if one of a node's
//      parents is invisible, then so is the node.
//  
{
    bool result = true;
    MDagPath searchPath( path );
    
    while ( true ) {
        if ( !isObjectVisible( searchPath )  ){
            result = false;
            break; 
        }   
        if ( searchPath.length() == 1 ) break;
        searchPath.pop();
    }   
    return result;
}   


bool isObjectSelected( const MDagPath & path )
//   
//  Description:
//      Check if the given object is selected
//  
{
	MDagPath sDagPath;

    MSelectionList activeList;
    MGlobal::getActiveSelectionList (activeList);

	MItSelectionList iter ( activeList, MFn::kDagNode );

	while ( !iter.isDone() )
	{
		if ( iter.getDagPath( sDagPath ) )
		{
			if ( sDagPath == path )
				return true;
		}
		iter.next();
	}

	return false;
}

bool isObjectOrParentSelected( const MDagPath & path )
//   
//  Description:
//      Check if this object and all of its parents are selected.
//  
{   
    bool result = false;
    MDagPath searchPath( path );
    
    while ( true ) {
        if ( isObjectSelected( searchPath )  ){
            result = true;
            break;
        }    
        if ( searchPath.length() <= 1 ) break;
        searchPath.pop();
    }   
    return result;
}

static char *NullName = "(Null)";
static char *UnknownType = "(Unknown)";

const char * objectName( MObject object )
//
//	Description:
//		return the name of the given object
//
{
	if( object.isNull() )
	{
		return NullName;
	}
	MStatus stat = MS::kSuccess;
	MFnDependencyNode dgNode;
	stat = dgNode.setObject( object );
	MString objectName = dgNode.name( &stat );
	if( MS::kSuccess != stat)
	{
		cerr << "Error: can not get the name of this object.\n";
		return UnknownType;
	}
	return objectName.asChar();
}
	
const char * objectType( MObject object )
//
//	Description:
//		return the type of the object
//
{
	if( object.isNull() )
	{
		return NullName;
	}
	MStatus stat = MS::kSuccess;
	MFnDependencyNode dgNode;
	stat = dgNode.setObject( object );
	MString typeName = dgNode.typeName( &stat );
	if( MS::kSuccess != stat)
	{
		cerr << "Error: can not get the type name of this object.\n";
		return UnknownType;
	}
	return typeName.asChar();
}

// Print functions for all the structs:
//
void printDtRGBA( DtRGBA &data )
{
#if 0
	cerr << "DtRGBA: " << data.r << " ";
	cerr << data.g << " " << data.b << " " << data.a << endl;
#endif
}

void printDtVec2f( DtVec2f &data )
{
#if 0 
	cerr << "DtVec2f: " << data.vec[0] << " ";
	cerr << data.vec[1] << endl;
#endif
}

void printDtVec3f( DtVec3f &data )
{
#if 0 
	cerr << "DtVec3f: " << data.vec[0] << " ";
	cerr << data.vec[1] << " " << data.vec[2] << endl;
#endif
}

void printGroupStruct( GroupStruct *data )
{
	if( NULL == data )
	{
		return;
	}

	cerr << "------ Content of GroupStruct:\n";
	cerr << "valid_bits: " << data->valid_bits << endl;	
	if( NULL != data->parentStruct )
	{
		cerr << "parentStruct: \n";
		printGroupStruct( data->parentStruct );
	}
	else
	{
		cerr << "parentStruct: NULL\n";
	}
	if( ! data->transformNode.isNull() )
	{
		cerr << "transformNode name: " << objectName( data->transformNode ) << endl;
	}
	if( ! data->shader.isNull() )
	{
		cerr << "shader name: " << objectName( data->shader ) << endl;
	}
	if( ! data->surfaceNode.isNull() )
	{
		cerr << "surfaceNode name: " << objectName( data->surfaceNode ) << endl;
	}
	cerr << "mtlName: " << data->mtlName << endl;
	cerr << "textureCount: " << data->textureCount << endl;
	if( ! data->textures.isNull() )
	{
		cerr << "textures name: " << objectName( data->textures ) << endl;
	}
	cerr << "textureName: " << data->textureName << endl;
	cerr << "------ End of GroupStruct Content\n";
}

void printFaceListStruct( FaceListStruct *data )
{
	if( NULL == data )
	{
		return;
	}

	if( NULL == data->list ) 
	{
		cerr << "list is EMPTY!\n";
		return;
	}

#if 1
	return;
#else
	cerr << "Content of FaceListStruct:\n";
	cerr << "count: " << data->count << endl;
	cerr << "indices: \n";
	for( int i = 0; i < data->count; i++ )
	{	
		cerr << data->list[i] << " ";
	}

	cerr << "\n------ End of FaceListStruct Content\n";
#endif

}
		

void printShapeStruct( ShapeStruct *data )
{
	if( NULL == data)
	{
		return;
	}

	cerr << "------ Content of ShapeStruct:\n";
	cerr << "valid_bits: " << data->valid_bits << endl;
	cerr << "instance: " << data->instance << endl;
	if( NULL != data->firstParentStruct )
	{
		cerr << "parentStruct: \n";
		printShapeStruct( data->firstParentStruct );
	}
	else
	{
		cerr << "parentStruct: NULL\n";
	}
	if( ! data->transformNode.isNull() )
	{
		cerr << "transformNode name: " << objectName( data->transformNode ) << endl;
	}
	if( ! data->surfaceNode.isNull() )
	{
		cerr << "surfaceNode name: " << objectName( data->surfaceNode ) << endl;
	}
	if( ! data->shapeNode.isNull() )
	{
		cerr << "shapeNode name: " << objectName( data->shapeNode ) << endl;
	}
	cerr << "groupCount: " << data->groupCount << endl;
	if( NULL != data->groupStructs )
	{
		cerr << "groupStructs: \n";
		printGroupStruct( data->groupStructs );
	}
	else
	{
		cerr << "groupStructs: NULL\n";
	}
	if( NULL != data->faceIdx )
	{
		cerr << "faceIdx:\n";
		printFaceListStruct( data->faceIdx );
	}
	if( NULL != data->stuvIdx )
	{
		cerr << "stuvIdx:\n";
		printFaceListStruct( data->stuvIdx );
	}
	if( NULL != data->normalIdx )
	{
		cerr << "normalIdx:\n";
		printFaceListStruct( data->normalIdx );
	}
	if( NULL != data->normPIdx )
	{
		cerr << "normPIdx:\n";
		printFaceListStruct( data->normPIdx );
	}
	if( NULL != data->usedGroups )
	{
		cerr << "usedGroups:\n";
		printFaceListStruct( data->usedGroups );
	}
	if( NULL != data->vertexList )
	{
		cerr << "vertexList:\n";
		for( int i = 0; i < data->vertexCount; i++ )
		{
			printDtVec3f( data->vertexList[i] );
		}
	}
	if( NULL != data->normalList )
	{
		cerr << "normalList:\n";
		for( int i = 0; i < data->normalCount; i++ )
		{
			printDtVec3f( data->normalList[i] );
		}
	}
	if( NULL != data->stuvList )
	{
		cerr << "stuvList:\n";
		for( int i = 0; i < data->stuvCount; i++ )
		{
			printDtVec2f( data->stuvList[i] );
		}
	}
	if( NULL != data->normalPList )
	{
		cerr << "normalPList:\n";
		for( int i = 0; i < data->normalPCount; i++ )
		{
			printDtVec3f( data->normalPList[i] );
		}
	}
	if( NULL != data->vertexColor )
	{
		// cerr << "vertexColor:\n";
	}
	cerr << "vertexCount: " << data->vertexCount << endl;
	cerr << "stuvCount: " << data->stuvCount << endl;
	cerr << "normalCount: " << data->normalCount << endl;
	cerr << "normalPCount: " << data->normalPCount << endl;
	cerr << "texCount: " << data->texCount << endl;
	cerr << "------ End of ShapeStruct Content\n";
}

void printDtPrivate( DtPrivate *data )
{	
	if( NULL == data )
	{
		return;
	}

	cerr << "------ Content of DtPrivate:\n";
	cerr << "shapeCount: " << data->shapeCount << endl;
	cerr << "shapes: \n";
	printShapeStruct( data->shapes );
	cerr << "texCount: " << data->texCount << endl;
	cerr << "------ End of DtPrivate Content\n";
}

// ===========================================================================
// ------------------------  PUBLIC  FUNCTIONS  ------------------------------
// ===========================================================================

int DtShapeTestFunc()
{
	printf("this is a test\n" );
	return 0;
}


//  ========== DtShapeGetCount ==========
/*
//	SYNPOSIS
//		Return the total number of shapes in the scene graph.
//
//	DESCRIPTION
//
          In the Maya API implementation of Dt, polysets in Maya scenes
          are converted into single shapes. Non-polygonal
          geometry is tesselated (according to the settings in the
          Render Stats window) with each resultant polyset generating
          a corresponding shape in Dt.

          In a hierarchical model, shapes may be generated which
          contain no geometry but have children. These shapes will
          have transformation information necessary for the
          positioning of their child shapes and proper output of the
          model as a whole.
*/
int  DtShapeGetCount()
{
    // make sure shapes have been loaded
    //
    if( NULL == local->shapes )
    {
		return( 0 );
    }

    // return shape count
    //
    return( local->shapeCount );

}  // DtShapeGetCount //


//  ========== DtShapeGetName ==========
/*
//  SYNOPSIS
//	Returns the name of the shape. 
//
//  DESCRIPTION
//
          DtShapeGetName() returns the shape name in the pointer to
          the variable name for the shape indicated by the shapeID
          parameter.  A read-only pointer to the name of the file is
          placed in the variable pointed to by name. This array is an
          internal buffer.  Do not free or modify the name.
*/

int  DtShapeGetName( int shapeID, char **name )
{

    MObject	node;
    char	*cp = NULL;

    static char	nameBuf[256]; 

    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) )
    {
		// Error: shape root should always have a name.
		//
        *name = NULL;
        return( 0 );  
    }

    // Get the node from the cache.
    //
    node = local->shapes[shapeID].transformNode;

	// Get the name from the node.
    //
    cp = (char *) objectName( node );
    if( NULL == cp ) 
	{
		*name = NULL;
		return( 0 );
	}

    // Copy over the name. No changes to the Maya name.
    //
    strcpy( nameBuf, cp );

    // Return name.
    //
    *name = nameBuf;

	// 
    return(1);

}  // DtShapeGetName //


//  ========== DtShapeGetParentID ==========
/*
//  SYNOPSIS
//  Returns the given shapes parent ID.  
//
//  DESCRIPTION
//
          DtShapeGetParentID() returns the parent ID for the given shape,
          shapeID in the pointer name. Can be used to find instances, or 
          other hierarchy information

          Will return -1 if there is no parent, or parents are not
          wanted.

         In case of multiple parents, this function returns the 
         ID of the first parent.
*/
int  DtShapeGetParentID( int shapeID )
{
    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) )
    {
        return -1;
    }

	// See if we want to return this info.
	//
	if( !DtExt_Parents() ) 
	{
		return -1;
	}

    // Get the shape nodes parent.
    //
    ShapeStruct *parentStruct = local->shapes[shapeID].firstParentStruct;

    // Find the parent ID.
    //
    if( NULL != parentStruct )
    {
        for( int i = 0; i < local->shapeCount; i++ )
        {
            if( parentStruct == &local->shapes[i] )
			{
				return i;
			}
        }
    }

    // Return -1 if parent is not found.
    //
    return -1;

}  // DtShapeGetParentID


//  ========== DtShapeGetParentName ==========
/*
//  SYNOPSIS
//  Returns the given shapes parent name.  
//
//  DESCRIPTION
//
          DtShapeGetParentName() returns the parent name for the given shape,
          shapeID in the pointer name.

          Will return NULL if there is no parent, or parents are not
          wanted.

  
          In case of multiple parents, this function returns the
          name of the first parent.
*/
void  DtShapeGetParentName( int shapeID, char **name )
{
    static char nameBuf[256];

    *name = NULL;

    //  See if we want to return this info.
    //
    if( !DtExt_Parents() ) 
	{
		return;
	}

    // Check for error.
    //
    if( (shapeID >= 0) && (shapeID < local->shapeCount) )
    {
        // Get the shape nodes parent.
        //
        ShapeStruct *parentStruct = local->shapes[shapeID].firstParentStruct;

        // If the parent exists then find its name and return.
        //
        if( NULL != parentStruct )
        {
			MObject parentNode = parentStruct->transformNode;

            // Get the name from the node.
            //
            char *cp = (char *) objectName( parentNode );

            if( NULL != cp )
            {
                // Return name.
                //
                strcpy( nameBuf, cp );
                *name = nameBuf;
            }
        }
    }

}  // DtShapeGetParentName



//  ========== DtExt_ShapeGetTransform ==========
//
//  SYNOPSIS
//  Returns the reference to MObject transform for given shapeID
//
//
int  DtExt_ShapeGetTransform(int shapeID, MObject &obj)
{
	if ((shapeID < 0) || (shapeID >= local->shapeCount))
    {
    	obj = MObject::kNullObj;
    	return(0);  // error, shape should always have a type
    }

	obj = local->shapes[shapeID].transformNode;
	
	return( 1 );
}

//  ========== DtExt_ShapeGetShapeNode ==========
//
//  SYNOPSIS
//  Returns the reference to MObject shape node for given shapeID
//
//  Get the 'surfaceNode' member of the shape's local (private) data
//  Similar to DtExt_ShapeGetOriginal but that accesses the
//  surfaceNode through the groupStructs
int  DtExt_ShapeGetShapeNode(int shapeID, MObject &obj)
{
	if ((shapeID < 0) || (shapeID >= local->shapeCount))
	{
		obj = MObject::kNullObj;
		return 0; // shape doesn't exist
	}

	obj = local->shapes[shapeID].surfaceNode;

	return 1;
}

//  ========== DtExt_ShapeGetShader ==========
//
//  SYNOPSIS
//  Returns the pointer to Shader (Material)
//
//
int DtExt_ShapeGetShader( int shapeID, int groupID, MObject &obj )
{
    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
    {
		// Error: shape should always have a type.
		//
		obj = MObject::kNullObj;
		return( 0 );  
    }

    // Find the material group node by group ID.
    //
    if( (groupID < 0) || (groupID >= local->shapes[shapeID].groupCount) )
	{
        return( 0 );
	}

    if( local->shapes[shapeID].groupCount == 0 ) 
	{
		// Error: groupCount should at least be 1.
		// why return 1?
		//
		return( 1 );
	}

    obj = local->shapes[shapeID].groupStructs[groupID].shader;

    return( 1 );
}


//  ========== DtExt_ShapeGetOriginal ==========
//
//  SYNOPSIS
//  Returns the pointer to Original MObject for the given shapeID
//
int  DtExt_ShapeGetOriginal(int shapeID, int groupID, MObject &obj)
{
    if( (shapeID < 0) || (shapeID >= local->shapeCount) )
    {
		obj = MObject::kNullObj;
		return(0);  
    }

    // Find the material group node by group ID.
    //
    if( (groupID < 0) || (groupID >= local->shapes[shapeID].groupCount) )
	{
        return(0);
	}
	if( local->shapes[shapeID].groupCount == 0 ) 
	{
		return( 1 ); // why return 1?
	}

    obj = local->shapes[shapeID].groupStructs[groupID].surfaceNode;

    return( 1 );
}


//  ========== DtExt_ShapeGetDagPath ==========
//
//  SYNOPSIS
//  Returns the pointer to DagPath for the given shapeID
//

int  DtExt_ShapeGetDagPath(int shapeID, MDagPath &dagPath)
{
    if( (shapeID < 0) || (shapeID >= local->shapeCount) )
    {
		cerr << "error in getting DagPath" << endl;
		
        return(0);  
    }   
   
    dagPath = *(local->shapes[shapeID].shapeDagPath);
    
    return( 1 );
}   

//  ========== DtExt_ShapeIsAnim ==========
//
//  SYNOPSIS
//  Returns True/False if Shape is Anim
//
int	DtExt_ShapeIsAnim( int shapeID )
{
    MObject obj;
    MStatus stat;
    MDagPath dagPath;
    
    obj = local->shapes[shapeID].surfaceNode;
    
    MFnDagNode currentDagNode( obj, &stat );
    
    stat = currentDagNode.getPath( dagPath );
    
    MFnDagNode fnDN( dagPath );
    
    // Check the inMesh attribute of the node
    //
    MPlug vPlug = fnDN.findPlug( "inMesh", &stat );
    
	if ( stat == MS::kSuccess )
	{
        if ( vPlug.isConnected( &stat ) )
			return true;
		else
			return false;
	} else {

		// Will need to extend this to see if the current node or higher 
		// have some type of animation on the transformations.
		// For now will just return false, i.e. no animation.

		return false;

	}


#if 0
	AlObject 	*obj;
	AlDagNode	*dagnode;
	AlChannel	*channel;
	AlAnimatable *animNode;

    if( (shapeID < 0) || (shapeID >= local->shapeCount) )
    {
    	return(0);  // error, shape should always have a type
    }

    obj = local->shapes[shapeID].original;	// get the original object
    dagnode = obj->asDagNodePtr();
	if( dagnode )
	{
		if( ( animNode = dagnode->asAnimatablePtr()) ) 
		{	// see if its anim
			channel = animNode->firstChannel();
			if ( AlIsValid( channel) )		// see if there is anything in it 
			{
				delete channel;
				ret = true;				
			}
		}
	}

	return( ret );

#endif

}

//  Used in DtExt_.h
//
//  ========== DtExt_ShapeIsInstanced ==========
//
//  SYNOPSIS
//  Returns the pointer to Instanced shape or -1 if not instanced
//
int DtExt_ShapeIsInstanced( int shapeID )
{
	int ret = -1;		

    if( (shapeID < 0) || (shapeID >= local->shapeCount) )
    {
    	return(-1);  // error, shape should always have a type
    }

    if( local->shapes[shapeID].instance != shapeID ) 
	{
		ret = local->shapes[shapeID].instance;
	}
    return( ret );
}

//  Used in DtExt_.h
//
//  ========== DtExt_ShapeIncTexCnt ==========
//
//  SYNOPSIS
//  Increments count of texture for this shape
//
int  DtExt_ShapeIncTexCnt( int shapeID )
{
    if( (shapeID < 0) || (shapeID >= local->shapeCount) )
    {
		return(0);  // error, shape should always have a type
    }

    local->shapes[shapeID].texCount++;

    return( 1 );
}

//  Used in DtExt_.h
//
//  ========== DtExt_ShapeGetTexCnt ==========
//
//  SYNOPSIS
//  Increments count of texture for this shape
//

int  DtExt_ShapeGetTexCnt( int shapeID, int *count )
{
    if( (shapeID < 0) || (shapeID >= local->shapeCount) )
    {
		*count = 0;
		return(0);  // error, shape should always have a type
    }

    *count = local->shapes[shapeID].texCount;

    return( 1 );
}

//  ========== DtShapeGetMatrix ==========
/*
//  SYNOPSIS
//	Return the transformation matrix for the shape.
//
//  DESCRIPTION
//
          DtShapeGetMatrix() returns the current shape matrix for the
          shape indicated by the shapeID parameter.  A read-only
          pointer to the 4x4 matrix is placed in the variable pointed
          to by matrix. The matrix for the shape is that of the
          current frame.  Setting a new frame will update the shape
          matrix see DtFrameSet.

          In case of multiple dag paths, this function returns the 
          global transformation matrix for the first dag path, if the
          outputTransform mode is kTRANSFORMMINIMAL.
*/
int  DtShapeGetMatrix( int shapeID, float **matrix )
{
    static float	mtxI[4][4];
	static float    mtxInclusive[4][4];
	//static float    mtxExclusive[4][4];
	static float    mtxLocal[4][4];

    // Initialize return values.
    //
    mtxI[0][0] = 1.0; mtxI[0][1] = 0.0; mtxI[0][2] = 0.0; mtxI[0][3] = 0.0;
    mtxI[1][0] = 0.0; mtxI[1][1] = 1.0; mtxI[1][2] = 0.0; mtxI[1][3] = 0.0;
    mtxI[2][0] = 0.0; mtxI[2][1] = 0.0; mtxI[2][2] = 1.0; mtxI[2][3] = 0.0;
    mtxI[3][0] = 0.0; mtxI[3][1] = 0.0; mtxI[3][2] = 0.0; mtxI[3][3] = 1.0;

    *matrix = (float *)&mtxI;		

    // Check for error
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		return(0);
	}

    // Get the Transform from the object.
    //
    MStatus stat = MS::kSuccess;
	MObject transformNode = local->shapes[shapeID].transformNode;
	
	// Take the first dag path.
	//
	MFnDagNode fnTransNode( transformNode, &stat );
	MDagPath dagPath;
	stat = fnTransNode.getPath( dagPath );

	MFnDagNode fnDagPath( dagPath, &stat );

	MMatrix mayaMatrix;
	MMatrix localMatrix;
	MMatrix testMatrix;

	switch( DtExt_outputTransforms() ) 
	{
		case kTRANSFORMNONE: // World: 

			break;

		// Without setting the path, it returns local transformation matrix.
		// With the path set, it returns the global transformation matrix.
		//
		case kTRANSFORMMINIMAL: // Flat
			mayaMatrix = dagPath.inclusiveMatrix();
			testMatrix = dagPath.exclusiveMatrix();
			localMatrix = fnTransNode.transformationMatrix( &stat );
			
			mayaMatrix.get( mtxInclusive );
			*matrix = (float *)&mtxInclusive;
		
#if 0

			if( DtExt_Debug() )
			{
				int i = 0;
				cerr << "inclusive transformation matrix:\n";
		
				for( i = 0; i < 4; i++ )
				{
					for( int j = 0; j < 4; j++ )
					{
						cerr << mtxInclusive[i][j] << " ";
					}
					cerr << endl;
				}

				testMatrix.get( mtxExclusive );
				cerr << "exclusive transformation matrix:\n";

				for( i = 0; i < 4; i++ )
				{
					for( int j = 0; j < 4; j++ )
					{
						cerr << mtxExclusive[i][j] << " ";
					}
					cerr << endl;
				}	
				localMatrix.get( mtxLocal );
				cerr << "local transformation matrix:\n";

				for( i = 0; i < 4; i++ )
				{
					for( int j = 0; j < 4; j++ )
					{
						cerr << mtxLocal[i][j] << " ";
					}
					cerr << endl;
				}
			}
#endif
			

			break;

		case kTRANSFORMALL: // FULL 
			mayaMatrix = fnTransNode.transformationMatrix( &stat );
			mayaMatrix.get( mtxLocal );
			*matrix = (float *)&mtxLocal;
		break;
	}

	return(1);

}  // DtShapeGetMatrix //


// 
//  ========== DtShapeGetRotationPivot ==========
//  SYNOPSIS
//      Returns the rotation pivot of the given shape in world coordinates.
//
int  DtShapeGetRotationPivot(int shapeID, float *x, float *y, float *z)
{
    // Check for error
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) )
    {
        return(0);
    }   
    
    // Get the Transform from the object.
    //
    MStatus stat;
    MObject transformNode = local->shapes[shapeID].transformNode;
    
    // Take the first dag path.
    //
    MFnDagNode fnTransNode( transformNode, &stat );
    MDagPath dagPath;
    stat = fnTransNode.getPath( dagPath );
    
    MFnDagNode fnDagPath( dagPath, &stat );

	MFnTransform transFn( dagPath );

	MPoint rP = transFn.rotatePivot( MSpace::kWorld, &stat );
	if ( stat == MS::kSuccess )
	{
		*x = rP.x;
		*y = rP.y;
		*z = rP.z;

		return 1;

	}

    return(0);

}   

// 
//  ========== DtShapeGetScalePivot ==========
/*
//  SYNOPSIS
//      Returns the scale pivot of the given shape.
//
//  DESCRIPTION
//
        DtShapeGetScalePivot() returns the scale pivot of the 
        given shape in world coordinates. The coordinates of the scale pivot
        are stored at the addresses of x, y and z. The scale pivot may not
        necessarily be at the origin of the object's transformation matrix.
*/
int  DtShapeGetScalePivot(int shapeID, float *x, float *y, float *z)
{

    // Check for error
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) )
    {
        return(0);
    }   
    
    // Get the Transform from the object.
    //
    MStatus stat;
    MObject transformNode = local->shapes[shapeID].transformNode;
    
    // Take the first dag path.
    //
    MFnDagNode fnTransNode( transformNode, &stat );
    MDagPath dagPath;
    stat = fnTransNode.getPath( dagPath );
    
    MFnDagNode fnDagPath( dagPath, &stat );
    
    MFnTransform transFn( dagPath );
    
    MPoint rP = transFn.scalePivot( MSpace::kWorld, &stat );
    if ( stat == MS::kSuccess )
    {
        *x = rP.x;
        *y = rP.y;
        *z = rP.z;
        
        return 1;
        
    }   

    return(0);
}  

// 
//  ========== DtShapeGetTranslation ==========
/*
//  SYNOPSIS
//      Returns the translation of the given shape.
//
//  DESCRIPTION
//
        DtShapeGetTranslation() returns the translation of the given shape's 
        local transformation matrix. The coordinates of the translation are
        stored at the addresses of x, y and z. 

*/
int  DtShapeGetTranslation(int shapeID, float *x, float *y, float *z)
{
    // Check for error
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) )
    {
        return(0);
    }   
    
    // Get the Transform from the object.
    //
    MStatus stat;
    MFnTransform transFn( local->shapes[shapeID].transformNode );
   
	MVector tP; 
    tP = transFn.translation( MSpace::kObject, &stat );

    if ( stat == MS::kSuccess )
    {
        *x = tP.x;
        *y = tP.y;
        *z = tP.z;
        
        return 1;
        
    }   

    return(0);
}   


// 
//  ========== DtShapeGetRotation ==========
/*
//  SYNOPSIS
//      Returns the rotation of the shape about the
//      x, y and z axes respectively.
//
//  DESCRIPTION
//
        DtShapeGetRotation() returns the rotation about the x, y, and z axes 
        in the given shape's local transformation matrix. The rotation angles
        (in degrees) are stored at the addresses of x, y and z. 

*/
int  DtShapeGetRotation(int shapeID, float *x, float *y, float *z)
{
    // Check for error
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) )
    {
        return(0);
    }   
    
    // Get the Transform from the object.
    //
    MStatus stat;
    MFnTransform transFn( local->shapes[shapeID].transformNode );

	double rP[3];    
	MTransformationMatrix::RotationOrder order = MTransformationMatrix::kXYZ;

    stat = transFn.getRotation( rP, order, MSpace::kObject );
    if ( stat == MS::kSuccess )
    {
        *x = rP[0];
        *y = rP[1];
        *z = rP[2];
        
        return 1;
        
    }   

    return(0);
}   


// 
//  ========== DtShapeGetScale ==========
/*
//  SYNOPSIS
//      Returns the scale of the shape.
//
//  DESCRIPTION
//
        DtShapeGetScale() returns the scale factors of
        the given shape's local transformation matrix. The scale factors
        are stored at the addresses of x, y and z. 

*/
int  DtShapeGetScale(int shapeID, float *x, float *y, float *z)
{
    // Check for error
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) )
    {
        return(0);
    }   
    
    // Get the Transform from the object.
    //
    MStatus stat;
    MFnTransform transFn( local->shapes[shapeID].transformNode );
   
	double sP[3]; 

    stat = transFn.getScale( sP );
    if ( stat == MS::kSuccess )
    {
        *x = sP[0];
        *y = sP[1];
        *z = sP[2];
        
        return 1;
        
    }   

    return(0);
}   

//
//  ========== DtShapeGetSetName ==========
/*
//  SYNOPSIS
//      Returns the set name of the given shape.
//
//  DESCRIPTION
//
          DtShapeGetSetName() returns the set name of the given shape,
          shapeID in the pointer name. The pointer name refers to an
          internal buffer and should not be deallocated.  If no set
          name is defined, name will contain a NULL pointer.

          THIS FUNCTION IS NOT IMPLEMENTED FOR MAYA

*/
int DtShapeGetSetName(int shapeID, char **name)
{
    // check for error
    //
    if ((shapeID < 0) || (shapeID >= local->shapeCount)) return(0);

	// *name = local->shapes[shapeID].setName;
	*name = NULL;

    return(1);
}   


//  ========== DtShapeGetVertexCount ==========
//
//  SYNOPSIS
/*
//	Return the vertex count for the given shape.
//
//  DESCRIPTION
//
          DtShapeGetVertexCount() returns the number of vertices in
          the shape indicated by the shapeID via the pointer to the
          count parameter.

*/
int  DtShapeGetVertexCount( int shapeID, int *count )
{
	int	ret = 0;
	int num = 0;

    // Initialize count.
    //
    *count = 0;

    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		return kFailure;
	}
    if( local->shapes[shapeID].groupCount == 0 ) 
	{			
		return 1; // why 1?
	}

	MStatus stat = MS::kSuccess;
	MObject shapeNode = local->shapes[shapeID].shapeNode;
	MFnMesh fnMidBody( shapeNode, &stat );
	if( MS::kSuccess == stat )
	{
		num = fnMidBody.numVertices( &stat );
	}
	else
	{
		DtExt_Err( "Error: this shape is not a polygon\n" );
	}

	if( num > 0 ) 
	{
       	*count = num;
		ret = 1;
	} 

    return ret;

}  // DtShapeGetVertexCount //


//  ========== DtShapeGetVertices ==========
/*
//  SYNOPSIS
//	Return the vertex list for the shape.
//
//  DESCRIPTION
//
          DtShapeGetVertices() returns a vertex list for the shape
          indicated by the given shapeID. The number of vertices is
          returned in count. A read-only pointer to a list of the
          vertices is placed in the variable pointed to by vertices.
          This array is an internal buffer.  Do not free or modify the
          vertex array.  C programmers can access the components of
          the vertex array as though it were a simple structure as
          follows:

                typedef struct DtVec3f { float vec[3];};

                DtVec3f *verts;
                DtShapeGetVertices( shapeID, DtVec3f &verts );
                // The first vertex 
                x = verts[0].vec[0];
                y = verts[0].vec[1];
                z = verts[0].vec[2];

*/
int  DtShapeGetVertices( int shapeID, int *count, DtVec3f **vertices )
{
    // Initialize return values.
    //
    *count    = 0;
    *vertices = NULL;

    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		return( 0 );
	}
	
	if( NULL != local->shapes[shapeID].vertexList )
	{
        // Return values.
        //
       	*count    = local->shapes[shapeID].vertexCount;
       	*vertices = local->shapes[shapeID].vertexList;
    }

    return 1;

}  // DtShapeGetVertices //


//  ========== DtShapeGetVertex ==========
/*
//  SYNOPSIS
//	Return the vertex for the given index.
//
//  DESCRIPTION
//
          DtShapeGetVertex() returns a vertex for the shape indicated
          by the given shapeID and index. The vertex is returned via
          the pointer to the vertex parameter.  C programmers can
          access the components of the vertex vector as though it were
          a simple structure as follows:

                typedef struct DtVec3f { float vec[3];};

                DtVec3f vert;
                x = vert.vec[0];
                y = vert.vec[1];
                z = vert.vec[2];

*/
int  DtShapeGetVertex( int shapeID, int index, DtVec3f *vertex )
{
    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		return(0);
	}

	if( index < local->shapes[shapeID].vertexCount )
	{
   	    // Set the vertex value.
        //
        DtVec3f a = local->shapes[shapeID].vertexList[index];

      	for( int i = 0; i < 3; i++) 
		{
       		vertex->vec[i] = a.vec[i];
		}
    }
    return 1;

}  // DtShapeGetVertex //

//  ========== DtShapeGetVerticesColor ==========
/*
//  SYNOPSIS
//  Return the vertex colour list for the shape.
//
//  DESCRIPTION
//
          DtShapeGetVerticesColor() returns a vertex color list for
          the shape indicated by the given shapeID. A read-only
          pointer to a list of the colors per vertex is placed in the
          variable pointed to by colors. The number of colors in the
          array is placed into the count parameter. The colors array
          contains a single color for each positional vector (see
          DtShapeGetVertices). These colors use the same indices
          as the positional vectors (see DtFaceGetIndexByShape,
          DtPolygonGetIndices)

          The colors returned from this function will the average
          vertex color.  If the vertex per face color is wanted see
          the DtShapeGetVerticesFaceColors() function.

          This array is an internal buffer.  Do not free or modify the
          color array.  C programmers can access the components of the
          color array as though it were a simple structure as follows:

               typedef struct { unsigned char r, g, b, a; } DtRGBA;

               DtRGBA *colors;
               int count;

               DtShapeGetVerticesColor( shapeID, &count, &colors);
               // The first vertex's color
               r = colors[0].r;
               g = colors[0].g;
               b = colors[0].b;
               a = colors[0].a;
*/

int  DtShapeGetVerticesColor( int shapeID, int *count, DtRGBA **colors )
{
    // Initialize return values.
    //
    *count  = 0;
    *colors	= NULL;

    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		return(0);
	}
    if( local->shapes[shapeID].vertexColor )
    {
        // Return values
        //
        *count    = local->shapes[shapeID].vertexCount;
        *colors   = local->shapes[shapeID].vertexColor;
    }

    return 1;

}  // DtShapeGetVerticesColor //

//  ========== DtShapeGetVertexColor ==========
/*
//  SYNOPSIS
//  Return the vertex colour for the given index.
//
//  DESCRIPTION
//
        DtShapeGetVertexColor() gets the vertex color for the
        given vertex index index of the shape shapeID. The
        red,green,blue and alpha values are stored in a structure of type 
        DtRGBA supplied by the caller and pointed to by color.
        The structure of DtRGBA is:

        struct DtRGBA {
                unsigned char r,g,b,a;
        }

*/
int  DtShapeGetVertexColor( int shapeID, int index, DtRGBA *color )
{
    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		return(0);
	}

    if( index < local->shapes[shapeID].vertexCount )
    {
		// Set the vertex value.
        //
        DtRGBA a = local->shapes[shapeID].vertexColor[index];

		color->r = a.r;
		color->g = a.g;
		color->b = a.b;
		color->a = a.a;
    }

    return 1;

}  // DtShapeGetVertexColor //


//  ========== DtShapeGetVerticesFaceColors ==========
/*
//  SYNOPSIS
//  Return the vertex list for the shape.
//
//  DESCRIPTION
//
          DtShapeGetVerticesFaceColors() returns a vertex color list for
          the shape indicated by the given shapeID. A read-only
          pointer to a list of the colors per vertex is placed in the
          variable pointed to by colors. The number of colors in the
          array is placed into the count parameter. The colors array
          contains a single color for each positional vector (see
          DtShapeGetVertices). These colors use the indices returned by
          the DtFaceGetColorIndexByShape function.

          The colors returned from this function will the vertex per face
          color.  

          This array is an internal buffer.  Do not free or modify the
          color array.  C programmers can access the components of the
          color array as though it were a simple structure as follows:

               typedef struct { unsigned char r, g, b, a; } DtRGBA;

               DtRGBA *colors;
               int count;

               DtShapeGetVerticesColor( shapeID, &count, &colors);
               // The first vertex's color
               r = colors[0].r;
               g = colors[0].g;
               b = colors[0].b;
               a = colors[0].a;
*/
int  DtShapeGetVerticesFaceColors( int shapeID, int *count, 
											DtFltRGBA **vfColors )
{
    // Initialize return values.
    //
    *count    = 0;
    *vfColors = NULL;

    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) )
    {
        return( 0 );
    }

    if( NULL != local->shapes[shapeID].vfColorList )
    {
        // Return values.
        //
        *count    = local->shapes[shapeID].vfColorCount;
        *vfColors = local->shapes[shapeID].vfColorList;
    }

    return 1;

}  // DtShapeGetVerticesFaceColors //



static bool addElement( MIntArray  *intArray, int newElem )
{
	unsigned int currIndex;

	for ( currIndex = 0; currIndex < intArray->length(); currIndex++ )
	{
		if ( newElem == (*intArray)[currIndex] ) // Don't add if it's there already
			return false;

		if ( newElem < (*intArray)[currIndex] )
		{
			intArray->insert( newElem, currIndex );
			return true;
		}
	}

	// If we made it here it should go at the end...
	intArray->append( newElem );
	return true;
}


int DtShapeGetTRSAnimKeys( int shapeID, MIntArray *keyFrames )
{
	MStatus			status;
	MObject			transformNode;

	MObject			anim;
	MFnDependencyNode dgNode;
	MDagPath		dagPath;

	int				currKey,
					numKeys,
					keyTime,
					stat;

	MItDependencyGraph::Direction direction = MItDependencyGraph::kUpstream;
	MItDependencyGraph::Traversal traversalType = MItDependencyGraph::kBreadthFirst;
	MItDependencyGraph::Level level = MItDependencyGraph::kNodeLevel;

	MFn::Type filter = MFn::kAnimCurve;
	
	// A quick check to see if the user has actually given us a valid
	// pointer.

	if ( !keyFrames )
	{
		return 0;
	}	
	
	stat = DtExt_ShapeGetTransform( shapeID, transformNode );
	if ( 1 != stat )
	{
		cerr << "DtExt_ShapeGetTransform problems" << endl;
		return 0;
	}
	
	MItDependencyGraph dgIter( transformNode, filter, direction, 
							traversalType, level, &status );

	for ( ; !dgIter.isDone(); dgIter.next() )
    {        
		anim = dgIter.thisNode( &status );
		MFnAnimCurve animCurve( anim, &status );
		if ( MS::kSuccess == status )
		{
			numKeys = animCurve.numKeyframes( &status );
			for ( currKey = 0; currKey < numKeys; currKey++ )
			{
				// Truncating values here...
				keyTime = (int) animCurve.time( currKey, &status ).value();
				addElement( keyFrames, keyTime );
			}
		}
    }

	return 1;
}

int DtShapeGetVtxAnimKeys( int shapeID, MIntArray *keyFrames )
{
	MStatus			status;
	MObject			shapeNode;

	MObject			anim;
	MFnDependencyNode dgNode;
	MDagPath		dagPath;

	int				currKey,
					numKeys,
					keyTime,
					stat;


	MDagPath		shapeDagPath;

	MItDependencyGraph::Direction direction = MItDependencyGraph::kUpstream;
	MItDependencyGraph::Traversal traversalType = MItDependencyGraph::kBreadthFirst;
	MItDependencyGraph::Level level = MItDependencyGraph::kNodeLevel;
	MFn::Type filter = MFn::kAnimCurve;

	// A quick check to see if the user has actually given us a valid
	// pointer.

	if ( !keyFrames )
	{
		return 0;
	}

	stat = DtExt_ShapeGetShapeNode( shapeID, shapeNode );
	if ( 1 != stat )
	{
		cerr << "Problems in shapeGetShapeNode" << endl;
		return 0;
	}

	MItDependencyGraph dgIter( shapeNode, filter, direction, 
							traversalType, level, &status );

	for ( ; !dgIter.isDone(); dgIter.next() )
	{
		anim = dgIter.thisNode( &status );
		MFnAnimCurve animCurve( anim, &status );
		if ( MS::kSuccess == status )
		{
			numKeys = animCurve.numKeyframes( &status );
			for ( currKey = 0; currKey < numKeys; currKey++ )
			{
				// Truncating values here; may need more control
				keyTime = (int) animCurve.time( currKey, &status ).value();
				addElement( keyFrames, keyTime );
			}
		}
	}

	return 1;

}


//  DtShapeGetVerticesAnimated:
/*
//  SYNOPSIS
//  Return the list of vertex indices that have animation applied to them
//
//  DESCRIPTION
          DtShapeGetVerticesAnimated() returns a list of vertex
          indices for the given shapeID. The number of vertices is
          returned in count. A pointer to a list of the indices
          is placed in the variable pointed to by vertices.
          It is the responsibility for the caller to deallocate the memory
          using the call: delete [] vertices. These indices 
          refer to vertices stored in the list returned from 
          DtShapeGetVertices or can be used to access a vertex
          directly using DtShapeGetVertex.

*/
int DtShapeGetVerticesAnimated(int shapeID, int *count, int **vertices)
{
    MObject obj;
    MStatus stat;
	MStatus userStat;
    MDagPath dagPath;

	bool	userAnimated = false;
    
    int num = 0;
    int *vertlist = NULL;

	// Lets check both the transform node and the shape node for the
	// user force animation dynamic attribute
	
	obj = local->shapes[shapeID].transformNode;
	MFnDagNode curTNode( obj, &stat );
	MPlug vPlug = curTNode.findPlug( "userAnimated", &userStat );

	if ( userStat == MS::kSuccess )
	{
		vPlug.getValue( userAnimated );
	}
	
    obj = local->shapes[shapeID].surfaceNode;
    
    MFnDagNode currentDagNode( obj, &stat );
    
    stat = currentDagNode.getPath( dagPath );
    
    MFnDagNode fnDN( dagPath );
    
    // Check to see if the user says this object is animated
    // based on the shape node

    vPlug = fnDN.findPlug( "userAnimated", &stat );
    
    if ( stat == MS::kSuccess )
	{
		vPlug.getValue( userAnimated );
	}
		

    // Check the input Mesh attribute of the node
    //
    vPlug = fnDN.findPlug( "inMesh", &stat );
    
	// Also check to see if there are any animCurves on the shape
	//
	if ( !local->shapes[shapeID].vtxAnimCheck ) 
	{
		local->shapes[shapeID].vtxAnimKeyFrames = new MIntArray;
		DtShapeGetVtxAnimKeys( shapeID, local->shapes[shapeID].vtxAnimKeyFrames );
		local->shapes[shapeID].vtxAnimCheck = true;
	}

    if ( userAnimated  
		 || local->shapes[shapeID].vtxAnimKeyFrames->length() > 0 
		 || (stat == MS::kSuccess && vPlug.isConnected( &stat ) ) )
	{		
        // Number of vertices.
        int numVerts = local->shapes[shapeID].vertexCount;

        // Set aside storage for maximum number of vertices.
        vertlist = new int[numVerts];

        if (vertlist) 
		{
            for (int i=0; i < numVerts; i++)
            {
				// Going to assume that all vertices are animated for now
				// to see if this works.  Can then figure out if the vertex
				// is part of a set/cluster that is animated later.

                vertlist[num++] = i;
                    
           }    
           
        }       

        // Ok, check if we found any animatable vertices.
        if (num > 0) 
		{
            *count = num;
            int *newvertlist = new int[num];
            
			for (int i = 0; i < num; i++)
                newvertlist[i] = vertlist[i];
            
			*vertices = newvertlist;
            
			delete [] vertlist;
            
			return ( 1 );
        }   
        
        // No animated vertices found.
        
        if ( vertlist )
            delete [] vertlist;
            
        *count = 0;
        *vertices = NULL;
        return ( 0 );

    } else {

		*count = 0;
    	*vertices = NULL;
    	return ( 0 );
	}

#if 0
	//
	// This code has been left here as a framework of including
	// the appropiate work for the Maya code base
	//

    // Get shape's AlObject
    AlObject *obj = local->shapes[shapeID].root;
    
    // Determine if it's a polysetNode
    AlPolysetNode *polysetNode = obj->asPolysetNodePtr();
    int num = 0;
    int *vertlist = NULL;
    if (polysetNode) {
        AlPolyset *polyset = polysetNode->polyset();
        
        // Number of vertices.
        int numVerts = local->shapes[shapeID].vertex_ct;
        
        // Set aside storage for maximum number of vertices.
        vertlist = new int[numVerts];
        if (vertlist) {
            for (int i=0; i < numVerts; i++)
            {
                AlPolysetVertex *vertex = polyset->vertex(i);
                if (vertex) {
                    AlChannel *channel = vertex->firstChannel();
                    AlClusterable *clusterable = vertex->asClusterablePtr();
                    AlCluster *cluster = NULL; 
                    if (clusterable) 
                        cluster = clusterable->firstCluster();
                           
                    if (channel) { // Channel animation on this vertex.
                        vertlist[num++] = i;
                        delete channel; 
                        if (cluster)
                            delete cluster;
                    }       
                    else if (cluster) { // Vertex animation on this.
                        vertlist[num++] = i;
                        delete cluster; 
                    }   
                    
               // Force vertex animation on world since this will include
               // transformation matrix and cluster animation
               
                    else if (DtOM_outputTransforms() == kTRANSFORMNONE)
                        vertlist[num++] = i;
                    }   
                    
                    delete vertex;
                }   
           }    

           if ( polyset )
                delete polyset;
        }

        // Ok, check if we found any animatable vertices.
        if (num > 0) {
            *count = num;
            int *newvertlist = new int[num];
            for (int i = 0; i < num; i++)
                newvertlist[i] = vertlist[i];
            *vertices = newvertlist;
            delete [] vertlist;
            return(1);
        }

        // No animated vertices found.

        if ( vertlist )
            delete [] vertlist;

        *count = 0;
        *vertices = NULL;
        return(0);

#endif

}


//  ========== DtShapeGetTextureVertexCount ==========
/*
//  SYNOPSIS
//	Return the texture vertex count for a given index.
//
//  DESCRIPTION
          DtShapeGetTextureVertexCount() returns the number of texture
          vertices in the shape indicated by the shapeID via the
          pointer to the count parameter.

*/
int  DtShapeGetTextureVertexCount( int shapeID, int *count )
{

    // Initialize count
    //
    *count = 0;

    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{	
		return(0);
	}

    // Get the number of vertices in the list and check it
    // with the given index.
    //
    *count = local->shapes[shapeID].stuvCount;

    return 1;

}  // DtShapeGetTextureVertexCount //


//  ========== DtShapeGetTextureVertices ==========
/*
//  SYNOPSIS
//	Return the texture vertex list for the shape.
//
//     DESCRIPTION
          DtShapeGetTextureVertices() returns a texture vertex list
          for the shape indicated by the given shapeID. The number of
          vertices is returned in count. A read-only pointer to a list
          of the texture vertices is placed in the variable pointed to
          by vertices. This array is an internal buffer.  Do not free
          or modify the texture vertex array.  C programmers can
          access the components of the texture vertex array as though
          it were a simple structure as follows:

               typedef struct DtVec2f {float vec[2];};

               DtVec2f *tVerts;
               DtShapeGetTextureVertices( shapeID, DtVec2f &tVerts );
               // The first texture vertex
               u = tVerts[0].vec[0];
               v = tVerts[0].vec[1];

*/
int  DtShapeGetTextureVertices( int shapeID, int *count, DtVec2f **vertices )
{
    // Initialize return values.
    //
    *count    = 0;
    *vertices = NULL;

    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		return(0);
	}

	if( local->shapes[shapeID].stuvCount > 0 )
	{
        // return values
    	//
		*count = local->shapes[shapeID].stuvCount;
       	*vertices = local->shapes[shapeID].stuvList;
    }

    return 1;

}  // DtShapeGetTextureVertices //


//  ========== DtShapeGetTextureVertex ==========
/*
//  SYNOPSIS
//	Return the texture vertex for a given index.
//
//  DESCRIPTION
//
          DtShapeGetTextureVertex() returns a texture vertex for the
          shape indicated by the given shapeID and index. The texture
          vertex is returned via the pointer to the vertex parameter.
          C programmers can access the components of the texture
          vertex vector as though it were a simple structure as
          follows:

               typedef struct DtVec2f { float vec[2];};

               DtVec2f tVert;
               u = tVert.vec[0];
               v = tVert.vec[1];

*/
int  DtShapeGetTextureVertex( int shapeID, int index, DtVec2f *vertex )
{
    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		return kFailure;
	}

	if( index < local->shapes[shapeID].stuvCount )
	{
       	DtVec2f uv = local->shapes[shapeID].stuvList[index];

       	// Return values
       	//
       	vertex->vec[0] = uv.vec[0];
       	vertex->vec[1] = uv.vec[1];
	} 
	else 
	{
		return( 0 );
	}

    return 1;

}  // DtShapeGetTextureVertex //


//  ========== DtShapeGetNormalCount ==========
/*
//  SYNOPSIS
//	Return the normal for the given index.
//
//  DESCRIPTION
//
          DtShapeGetNormalCount() returns the number of normals in the
          shape indicated by the shapeID via the pointer to the count
          parameter.

*/
int  DtShapeGetNormalCount( int shapeID, int *count )
{
    // Initialize count.
    //
    *count = 0;

    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		return(0);
	}
   	const int num = local->shapes[shapeID].normalCount;

	if( num > 0 ) 
    {
       	*count = num;
    }

    return 1;

}  // DtShapeGetNormalCount //


//  ========== DtShapeGetNormals ==========
/*
//  SYNOPSIS
//	Return the normals list for the shape.
//
//  DESCRIPTION
//
          DtShapeGetNormals() returns a normal list for the shape
          indicated by the given shapeID. The number of normals is
          returned in count. A read-only pointer to a list of the
          normals is placed in the variable pointed to by normals.
          This array is an internal buffer.  Do not free or modify the
          normal array.  C programmers can access the components of
          the normal array as though it were a simple structure as
          follows:

               typedef struct DtVec3f { float vec[3];};

               DtVec3f *norms;
               DtShapeGetNormals( shapeID, DtVec3f &norms );
               // The first normal
               x = norms[0].vec[0];
               y = norms[0].vec[1];
               z = norms[0].vec[2];
*/
int  DtShapeGetNormals( int shapeID, int *count, DtVec3f **normals )
{
    // Initialize return values.
    //
    *count   = 0;
    *normals = NULL;

    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		return(0);
	}
    if( local->shapes[shapeID].normalList )
    {
        // Return values:
        //
        *count   = local->shapes[shapeID].normalCount;
        *normals = local->shapes[shapeID].normalList;
    }

    return 1;

}  // DtShapeGetNormals //


//  ========== DtShapeGetNormal ==========
/*
//  SYNOPSIS
//	Return the normal for the given index.
//
//  DESCRIPTION
//
          DtShapeGetNormal() returns a normal for the shape indicated
          by the given shapeID and index. The noraml is returned via
          the pointer to the vertex parameter.  C programmers can
          access the components of the vertex vector as though it were
          a simple structure as follows:

               typedef struct DtVec3f { float vec[3];};

               DtVec3f norm;
               x = norm.vec[0];
               y = norm.vec[1];
               z = norm.vec[2];
*/
int  DtShapeGetNormal( int shapeID, int index, DtVec3f *normal )
{
    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		return kFailure;
	}
    if( index < local->shapes[shapeID].normalCount )
    {
        // Get the normal value.
		//
        DtVec3f a = local->shapes[shapeID].normalList[index];

        // Set the vertex value.
        //
        for( int i = 0; i < 3; i++)
		{
        	normal->vec[i] = a.vec[i];
		}
    }

    return 1;

}  // DtShapeGetNormal //

//  ========== DtShapeGetPolygonNormalCount ==========
/*
//  SYNOPSIS
//  Places the number of polygon normals into the count parameter.
//
//  DESCRIPTION
//
          DtShapeGetPolygonNormalCount() returns the number of polygon
          normals in the shape indicated by the shapeID via the pointer 
          to the count parameter.
          
          This corresponds to the flat shaded shaders.
*/
int  DtShapeGetPolygonNormalCount(int shapeID, int *count)
{

    // Initialize count
    //
    *count = 0;

    // check for error
    //
    if ((shapeID < 0) || (shapeID >= local->shapeCount)) return(0);

    const int num = local->shapes[shapeID].normalPCount;

    if( num > 0 )
    {
        *count = num;
    }

    return 1;

}  // DtShapeGetPolygonNormalCount //

//  ========== DtShapeGetPolygonNormals ==========
/*
//  SYNOPSIS
//  Return the normals list for the shape.
//
//     DESCRIPTION
          DtShapeGetPolygonNormals() returns a polygon normal list for the shape
          indicated by the given shapeID. The number of normals is
          returned in count. A read-only pointer to a list of the
          normals is placed in the variable pointed to by normals.
          This array is an internal buffer.  Do not free or modify the
          normal array.  C programmers can access the components of
          the normal array as though it were a simple structure as
          follows:

               typedef struct DtVec3f { float vec[3];};

               DtVec3f *norms;
               DtShapeGetNormals( shapeID, DtVec3f &norms );
               // The first normal
               x = norms[0].vec[0];
               y = norms[0].vec[1];
               z = norms[0].vec[2];

           This is for flat shaded polygons
*/
int  DtShapeGetPolygonNormals(int shapeID, int *count, DtVec3f **normals)
{
    // initialize return values
    //
    *count   = 0;
    *normals = NULL;
   
    // check for error
    //
    if ((shapeID < 0) || (shapeID >= local->shapeCount)) 
	{
		return(0);
	}
    
    if ( local->shapes[shapeID].normalPList )
    {
        // return values
        //
        *count   = local->shapes[shapeID].normalPCount;
        *normals = local->shapes[shapeID].normalPList;
    }   
    
    return 1;
    
}  // DtShapeGetPolygonNormals //

//  ========== DtShapeGetPolygonNormal ==========
/*
//  SYNOPSIS
//  Return the normal for the given index.
//
//  DESCRIPTION
//
          DtShapeGetPolygonNormal() returns a polygon normal for the 
          shape indicated by the given shapeID and index. The noraml 
          is returned via the pointer to the vertex parameter.  
          C programmers can access the components of the vertex vector 
          as though it were a simple structure as follows:

                typedef struct DtVec3f { float vec[3];};

                DtVec3f norm;
                x = norm.vec[0];
                y = norm.vec[1];
                z = norm.vec[2];

          This is for flat shaded polygons
*/
int  DtShapeGetPolygonNormal(int shapeID, int index, DtVec3f *normal)
{

    // check for error
    //
    if ((shapeID < 0) || (shapeID >= local->shapeCount)) return(0);
    
    if ( index < local->shapes[shapeID].normalPCount )
    {
        // get the normal value
        //
        DtVec3f a = local->shapes[shapeID].normalPList[index];
        
        // Set the vertex value.
        //
        for ( int i = 0; i < 3; i++)
            normal->vec[i] = a.vec[i];
    }       
    
    return 1;
    
}  // DtShapeGetPolygonNormal //

//  ========== DtShapeGetPolygonNormalIdx ==========
/*
//  SYNOPSIS
//  Return the index list for the normals of a face.
//
//  DESCRIPTION
//
        DtShapeGetPolygonNormalIdx will return a list of indices that
        can be used to relate the polygon face index to a Polygon Normal
        in the polygon Normal list

*/
int  DtShapeGetPolygonNormalIdx( int shapeID, int groupID, int *count, int **indices)
{   
    // Initialize return values.
    //
    *count   = 0;
    *indices = NULL;
    
    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) )
    {   
        return kFailure;
    }
    if( (groupID < 0) || (groupID >= local->shapes[shapeID].groupCount) )
    {   
        return kFailure;
    }
    if( local->shapes[shapeID].groupCount == 0 )
    {   
        return( 1 ); // Shapes may not have any groups so not a real error
    }
    
    // Get the list of normal indicies.
    //
    const int num = local->shapes[shapeID].normPIdx[groupID].count;
    const int *list = local->shapes[shapeID].normPIdx[groupID].list;
    
    if( DtExt_Debug() )
    {   
        printFaceListStruct( &local->shapes[shapeID].normPIdx[groupID] );
    }
    
    // kludge, test if only index is -1 (comment from PA DT).
    //
    if( (num == 1) && (list[0] == -1) )
    {   
        return kSuccess;
    }
    
    // Return values.
    //
    *count = num;
    *indices = (int *)list;
    
    return 1;

}  /* DtShapeGetPolygonNormalIdx */



//  ========== DtShapeGetChildren ==========
/*
//  SYNOPSIS
//  Return a list of children *their shape IDs) for the Given Shape.
//	Returns a list that the user should free themselves when finished - 
// 
//  DESCRIPTION
//
        DtShapeGetChildren() gets a list of the children of 
        the given shape shapeID. The size of the list is stored 
        in the address of count and the list of shape ID's of the
        children is stored in the buffer children. The caller 
        should free this list when they are done using a call like: 
        free(children). A shape will have children if the full 
        hierarchy mode is chosen for export.

     This shape has to be the first parent of its children
     according to this implementation, i.e., if a transform node
     is the second parent of a certain node, it will not be reported of
     having this child.
   
     Will work around this by checking the parentArray of the child.

*/
int  DtShapeGetChildren( int shapeID, int *count, int **children )
{
	// Assume that there are no children.
	//
	*count = 0;
	*children = NULL;

    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		return(0);
	}

    // If we don't want parents then we don't want children either.
    //
    if( !DtExt_Parents() )
    {
        return 1;
    }   

    *children = (int *)malloc( local->shapeCount * sizeof( int ));

    ShapeStruct *shape = &local->shapes[shapeID];

	for( int i = 0; i < local->shapeCount; i++ )
	{
		if( local->shapes[i].firstParentStruct == shape )
		{
			(*children)[*count] = i;
			(*count)++;
		}
	}

	// If there is any children reallocate arrary to size and return
	// else free memory and return.
	//
	if( *count )
	{
		*children = (int *)realloc( *children, *count*sizeof( int ) );
	} 
	else 
	{
		free( *children );
		*children = NULL;
	}
	return 1;
}

//  ========== DtShapeIsDoubleSided ==========
/*
//  SYNOPSIS
//  Determine whether the polygons of a shape are double sided.
//
//  DESCRIPTION
//
          DtShapeIsDoubleSided() tells if the polygons of the given
          shape are double sided as it was set in the 'Render stats'
          window.

*/
int DtShapeIsDoubleSided( int shapeID )
{
    MObject obj;
	MStatus stat;
    MDagPath dagPath;
 
    obj = local->shapes[shapeID].surfaceNode;

	MFnDagNode currentDagNode( obj, &stat );

	stat = currentDagNode.getPath( dagPath );

    MFnDagNode fnDN( dagPath );

    // Check the visibility attribute of the node
    //
    MPlug vPlug = fnDN.findPlug( "doubleSided" );

    bool doublesided;
    vPlug.getValue( doublesided );

    return  doublesided;

} // DtShapeIsDoubleSided

//  ========== DtShapeIsOpposite ==========
//
//  SYNOPSIS
//  Determine whether the polygons of a shape are double sided.
//
int DtShapeIsOpposite( int shapeID )
{
    MObject obj;
    MStatus stat;
    MDagPath dagPath;

    obj = local->shapes[shapeID].surfaceNode;

    MFnDagNode currentDagNode( obj, &stat );

    stat = currentDagNode.getPath( dagPath );

    MFnDagNode fnDN( dagPath );

    // Check the opposite attribute of the node
    //
    MPlug vPlug = fnDN.findPlug( "opposite" );

    bool opposite;
    vPlug.getValue( opposite );

    return  opposite;

} // DtShapeIsOpposite

//  ========== DtShapeIsFlatShaded ==========
//
//  SYNOPSIS
//  Determine whether the polygons of a shape are Flat Shaded.
//
int DtShapeIsFlatShaded( int shapeID )
{
    MObject obj;
    MStatus stat;
    MDagPath dagPath;
    
    obj = local->shapes[shapeID].surfaceNode;
    
    MFnDagNode currentDagNode( obj, &stat );
    
    stat = currentDagNode.getPath( dagPath );
    
    MFnDagNode fnDN( dagPath );
    
    // Check the smoothShading attribute of the node
    //
    MPlug vPlug = fnDN.findPlug( "smoothShading" );
    
    bool smooth;
    vPlug.getValue( smooth );
   
    return  !smooth;
    
} // DtShapeIsFlatShaded

// ========================================================
// ==================  Group Routines =====================
// ========================================================


//  ========== DtGroupGetName ==========
//
//  SYNOPSIS
//  Return the group name.  The group name is the material name.
//
void  DtGroupGetName( int shapeID, int groupID, char **name )
{
    static char nameBuf[64];
    *name = NULL;

    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) )
    {
        return; // Error: shape root should always have a name.
    }

    // Get the shape node.
    //
    ShapeStruct *shape = &local->shapes[shapeID];

    // Check if the group is out of range.
    //
    if( (groupID >= 0) && (groupID < shape->groupCount) )
    {
        GroupStruct *group = &shape->groupStructs[groupID];

        const char *cp = objectName( group->shader );

        if( cp )
        {
            // return name
            //
            strcpy( nameBuf, cp );
            *name = nameBuf;
        }
		else 
		{
			DtExt_Err( "Shader does not exist\n" );
		}
    }
}  // DtGroupGetName


//  ========== DtGroupGetMatrix ==========
//
//  SYNOPSIS
//  Return the transformation matrix for a given group
//  within a kinematics shape.
//
//      NOTE: return code 0 = error
//						  1 = ok
//						  2 = Identity 
//
//  Q: Not used? - why still in Dt.h?
// 
int  DtGroupGetMatrix(int shapeID, int groupID, float **matrix )
{
    static float  mtx[4][4];

    // Initialize return values.
    //
    mtx[0][0] = 1.0; mtx[0][1] = 0.0; mtx[0][2] = 0.0; mtx[0][3] = 0.0;
    mtx[1][0] = 0.0; mtx[1][1] = 1.0; mtx[1][2] = 0.0; mtx[1][3] = 0.0;
    mtx[2][0] = 0.0; mtx[2][1] = 0.0; mtx[2][2] = 1.0; mtx[2][3] = 0.0;
    mtx[3][0] = 0.0; mtx[3][1] = 0.0; mtx[3][2] = 0.0; mtx[3][3] = 1.0;

    // Return pointer to the composite matrix.
    //
    *matrix = (float *)&mtx;

    return( 2 );  // ALWAYS returns and Identity Matrix

}  // DtGroupGetMatrix //


//  ========== DtGroupGetCount ==========
//
//  SYNOPSIS
//	Return the number of groups in the shape.
//

int  DtGroupGetCount( int shapeID )
{
    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) )
    {
	    return 0;
    }

    // Return number of groups.
    //
    return local->shapes[shapeID].groupCount;

}  // DtGroupGetCount //


//  ========== DtGroupGetParentID ==========
//
//  SYNOPSIS
//  Return the groups parent groupID.
//
int  DtGroupGetParentID( int shapeID, int /* groupID */ )
{
	return DtShapeGetParentID( shapeID );

}  // DtGroupGetParentID

//  ========== DtGroupGetParentName ==========
//
//  SYNOPSIS
//  Return the groups parent group name.
//
void  DtGroupGetParentName( int shapeID, int groupID, char **name )
{
    int parentID = DtGroupGetParentID( shapeID, groupID );

    DtMtlGetName( shapeID, parentID, name);

}  // DtGroupGetParentName


//  ========== DtGroupGetVertexCount ==========
//
//  SYNOPSIS
//  Return the vertex count for the given shapes group.
//
int  DtGroupGetVertexCount( int shapeID, int groupID, int *count )
{
    // Initialize count.
    //
    *count = 0;

    // Check for error.
    //
    if( (shapeID >= 0) && (shapeID < local->shapeCount) )
    {
    	ShapeStruct *shape = &local->shapes[shapeID];

        if( (groupID >= 0) && (groupID < shape->groupCount) )
    	{
			if ( local->shapes[shapeID].vertexList )
			{
				int faceCnt = 0;
				// Get the number of vertices in the shape.
				//
				int vertexCnt = local->shapes[shapeID].vertexCount;

				// Get the indices for the group.
				//
				FaceListStruct face = shape->faceIdx[groupID];

				// Determine the number of unique vertices in the index list.
				//
				if ( (faceCnt = face.count) )
				{
                    int *indices = face.list;
					char *vertexList = ( char * ) calloc ( vertexCnt, sizeof( char ));

					for ( int i = 0; i < faceCnt; i++ )
					{
						if ( indices[i] != DtEND_OF_FACE )
						{
							if ( vertexList[ indices[i] ] == 0 )
							{
								vertexList[ indices[i] ] = 1;
								(*count)++;
							}
						}
					}

					// Free the allocated memory.
					//
					if ( vertexList )
					{
						free( vertexList );
					}
				}
            return 1;
			}
		}
    }

    return 0;

}  // DtGroupGetVertexCount

//  ========== DtGroupGetVertices ==========
//
//  SYNOPSIS
//  Return the vertex list for the given shapes group.
//
//      NOTE :This routine allocates an array which will need to
//      be freed by the user.
//
int  DtGroupGetVertices( int shapeID, int groupID, int *count, DtVec3f **vertices)
{
    // initialize return values
    //
    *count    = 0;
    *vertices = NULL;

    // check for error
    //
    if ((shapeID >= 0) && (shapeID < local->shapeCount))
    {
		ShapeStruct *shape = &local->shapes[shapeID];

		if ( (groupID >= 0) && (groupID < shape->groupCount) )
		{	

			if ( shape->vertexList )
			{
				int faceCnt;

				// Get the number of vertices in the shape.
				//
				int vertexCnt = shape->vertexCount;

				// Get the indices for the group.
				//
				FaceListStruct face = shape->faceIdx[groupID];

				// Determine the number of unique vertices in the index list.
				//
				if( (faceCnt = face.count) )
				{
					int *indices = face.list;
					char *vertexList = ( char * ) calloc ( vertexCnt, sizeof( char ));

					// Create a new vertex list.
					//
					*vertices = ( DtVec3f * ) malloc ( vertexCnt * sizeof( DtVec3f ));
					DtVec3f *vList = *vertices;

					for ( int i = 0; i < faceCnt; i++ )
					{
						if ( indices[i] != DtEND_OF_FACE )
						{
							if ( vertexList[ indices[i] ] == 0 )
							{
								const DtVec3f vert = shape->vertexList[ indices[i] ];

								// Set the vertex value.
								//
								for ( int j = 0; j < 3; j++)
									vList->vec[j] = vert.vec[j];

								vList++;

								vertexList[ indices[i] ] = 1;
								(*count)++;
							}
						}
					}

					// Free allocated list.
					//
					if ( vertexList )
					{
						free ( vertexList );
					}

					*vertices = ( DtVec3f * ) realloc ( *vertices, *count * sizeof( DtVec3f ));
				}

				return 1;
			}
		}
    }

    return 0;

}  // DtGroupGetVertices


//  ========== DtGroupGetVertex ==========
//
//  SYNOPSIS
//  Return the vertex for the given index.
//
int  DtGroupGetVertex( int shapeID, int groupID, int index, DtVec3f *vertex)
{

    // check for error
    //
    if ((shapeID >= 0) && (shapeID < local->shapeCount))
    {
		ShapeStruct *shape = &local->shapes[shapeID];

        if ( (groupID >= 0) && (groupID < shape->groupCount) )
		{

			if ( shape->vertexList )
			{
				int faceCnt;

				// Get the number of vertices in the shape.
				//
				int vertexCnt = shape->vertexCount;

				// Get the indices for the group.
				//
				FaceListStruct face = shape->faceIdx[groupID];

				// Determine the number of unique vertices in the index list.
				//
				if ( (faceCnt = face.count) )
				{
					int   count      = 0;
					int  *indices    = face.list;
					char *vertexList = ( char * ) calloc ( vertexCnt, sizeof( char ));

					for ( int i = 0; i < faceCnt; i++ )
					{
						if ( indices[i] != DtEND_OF_FACE )
						{
							if ( vertexList[ indices[i] ] == 0 )
							{
								if ( count == index )
								{
									const DtVec3f vert = shape->vertexList[ indices[i] ];

									// Set the vertex value.
									//
									for ( int j = 0; j < 3; j++)
										vertex->vec[j] = vert.vec[j];

									break;
								}

								vertexList[ indices[i] ] = 1;
								count++;
							}
						}
					}

					// Free allocated list.
					//
					if ( vertexList )
						free ( vertexList );
				}

				return 1;
			}
		}
    }

    return 0;

}  // DtGroupGetVertex

//  ========== DtGroupGetTextureVertexCount ==========
//
//  SYNOPSIS
//  Return the vertex count for the given shape.
//
int  DtGroupGetTextureVertexCount( int shapeID, int groupID, int *count )
{
    // Initialize count.
    //
    *count = 0;

    // Check for error.
    //
    if( (shapeID >= 0) && (shapeID < local->shapeCount) )
    {
		ShapeStruct *shape = &local->shapes[shapeID];

        if( (groupID >= 0) && (groupID < shape->groupCount) )
		{
			if ( shape->stuvList )
			{
				int faceCnt = 0;

				// Get the number of vertices in the shape.
				//
				int vertexCnt = shape->stuvCount;

				// Get the indices for the group.
				//
				FaceListStruct face = shape->stuvIdx[groupID];

				// Determine the number of unique vertices in the index list.
				//
				if( (faceCnt = face.count) )
				{
					int *indices    = face.list;
					char *vertexList = ( char * ) calloc ( vertexCnt, sizeof( char ));

					for( int i = 0; i < faceCnt; i++ )
					{
						if( indices[i] != DtEND_OF_FACE )
						{
							if( vertexList[ indices[i] ] == 0 )
							{
								vertexList[ indices[i] ] = 1;
								(*count)++;
							}
						}
					}

					// Free the allocated memory.
					//
					if( vertexList )
					{
						free( vertexList );
					}
				}	
			}
			return 1;
		}
    }

    return 0;

}  // DtGroupGetTextureVertexCount


//  ========== DtGroupGetTextureVertices ==========
//
//  SYNOPSIS
//  Return the texture vertex list for the group.
//
//  NOTE :This routine allocates an array which will need to
//        be freed by the user.
//
//  Q: anyway to avoid this?
//
int  DtGroupGetTextureVertices( int shapeID, int groupID, int *count, DtVec2f **vertices )
{
    // Initialize return values.
    //
    *count    = 0;
    *vertices = NULL;

    // Check for error.
    //
    if( (shapeID >= 0) && (shapeID < local->shapeCount) )
    {
		ShapeStruct *shape = &local->shapes[shapeID];

        if( (groupID >= 0) && (groupID < shape->groupCount) )
		{
			if( shape->stuvList )
			{
				int faceCnt = 0;

				// Get the number of vertices in the shape.
				//
				int vertexCnt = shape->stuvCount;

				// Get the indices for the group.
				//
				FaceListStruct face = shape->stuvIdx[groupID];

				// Determine the number of unique vertices in the index list.
				//
				if( (faceCnt = face.count) )
				{
					int *indices    = face.list;
					char *vertexList = ( char * ) calloc ( vertexCnt, sizeof( char ));

					// Create a new vertex list.
					//

					*vertices = ( DtVec2f * ) malloc ( faceCnt * sizeof( DtVec2f ));
					DtVec2f *uvList = *vertices;

					for( int i = 0; i < faceCnt; i++ )
					{
						if( indices[i] != DtEND_OF_FACE )
						{
							if( vertexList[ indices[i] ] == 0 )
							{
								const DtVec2f uv = shape->stuvList[ indices[i] ];

								// Set the vertex value.
								//
								uvList->vec[0] = uv.vec[0];
								uvList->vec[1] = uv.vec[1];

								uvList++;

								vertexList[ indices[i] ] = 1;
								(*count)++;
							}
						}
					}

					// Free allocated list.
					//
					if( vertexList )
					{
						free ( vertexList );
					}

					*vertices = (DtVec2f *)realloc( *vertices, 
												*count * sizeof( DtVec2f ));
				}
			}
			return 1;
		}
    }

    return 0;

}  // DtGroupGetTextureVertices


//  ========== DtGroupGetTextureVertex ==========
//
//  SYNOPSIS
//  Return the texture vertex for the given index.
//
int  DtGroupGetTextureVertex( int shapeID, int groupID, int index, DtVec2f *vertex )
{
    // Check for error.
    //
    if( (shapeID >= 0) && (shapeID < local->shapeCount) )
    {
		ShapeStruct *shape = &local->shapes[shapeID];

        if( (groupID >= 0) && (groupID < shape->groupCount) )
		{
			if( shape->stuvList )
			{
				int faceCnt = 0;

				// Get the number of vertices in the shape.
				//
				int vertexCnt = shape->stuvCount;

				// Get the indices for the group.
				//
				FaceListStruct face = shape->stuvIdx[groupID];

				// Determine the number of unique vertices in the index list.
				//
				if( (faceCnt = face.count) )
				{
					int   count      = 0;
					int  *indices    = face.list;
					char *vertexList = (char *)calloc( vertexCnt, sizeof( char ));

					for( int i = 0; i < faceCnt; i++ )
					{
						if( indices[i] != DtEND_OF_FACE )
						{
							if( vertexList[ indices[i] ] == 0 )
							{
								if( count == index )
								{
									const DtVec2f uv = shape->stuvList[ indices[i] ];

									// Set the vertex value.
									//
									vertex->vec[0] = uv.vec[0];
									vertex->vec[1] = uv.vec[1];
									break;
								}
								vertexList[ indices[i] ] = 1;
								count++;
							}
						}
					}
					// Free allocated list.
					//
					if( vertexList )
					{
						free ( vertexList );
					}
				}
			}
			return 1;
		}
    }

    return 0;

}  // DtGroupGetTextureVertex


//  ========== DtGroupGetNormalCount ==========
//
//  SYNOPSIS
//  Return the normal count for the given shapes group.
//
int  DtGroupGetNormalCount( int shapeID, int groupID, int *count )
{
    // Initialize count.
    //
    *count = 0;

    // Check for error.
    //
    if( (shapeID >= 0) && (shapeID < local->shapeCount) )
    {
		ShapeStruct   *shape = &local->shapes[shapeID];

        if( (groupID >= 0) && (groupID < shape->groupCount) )
		{
			if( shape->normalList )
			{
				int  faceCnt = 0;
				
				// Get the number of vertices in the shape.
				//
				int normalCnt = shape->normalCount;

				// Get the indices for the group.
				//
				FaceListStruct face = shape->normalIdx[groupID];

				// Determine the number of unique vertices in the index list.
				//
				if( (faceCnt = face.count) )
				{
					int  *indices    = face.list;
					char *vertexList = (char *)calloc( normalCnt, sizeof( char ));

					for( int i = 0; i < faceCnt; i++ )
					{
						if( indices[i] != DtEND_OF_FACE )
						{
							if( vertexList[ indices[i] ] == 0 )
							{
								vertexList[ indices[i] ] = 1;
								(*count)++;
							}
						}
					}

					// Free the allocated memory.
					//
					if( vertexList )
					{
						free( vertexList );
					}
				}
			}
			return 1;
		}
    }

    return 0;

}  // DtGroupGetNormalCount


//  ========== DtGroupGetNormals ==========
//
//  SYNOPSIS
//  Return the normal list for the given shapes group.
//
//  NOTE :This routine allocates an array which will need to
//        be freed by the user.
//
//  Q: anyway to avoid this?
//
int  DtGroupGetNormals( int shapeID, int groupID, int *count, DtVec3f **vertices )
{
    // Initialize return values.
    //
    *count    = 0;
    *vertices = NULL;

    // Check for error.
    //
    if( (shapeID >= 0) && (shapeID < local->shapeCount) )
    {
		ShapeStruct *shape = &local->shapes[shapeID];

        if( (groupID >= 0) && (groupID < shape->groupCount) )
		{
			if( shape->normalList )
			{
				int  faceCnt = 0;

				// Get the number of vertices in the shape.
				//
				int normalCnt = shape->normalCount;

				// Get the indices for the group.
				//
				FaceListStruct face = shape->normalIdx[groupID];

				// Determine the number of unique vertices in the index list.
				//
				if( (faceCnt = face.count) )
				{
					int  *indices    = face.list;
					char *vertexList = (char *)calloc ( normalCnt, sizeof( char ));

					// Create a new vertex list.
					//
					*vertices = (DtVec3f *)malloc ( faceCnt * sizeof( DtVec3f ));
					DtVec3f *vList = *vertices;

					for( int i = 0; i < faceCnt; i++ )
					{
						if( indices[i] != DtEND_OF_FACE )
						{
							if( vertexList[ indices[i] ] == 0 )
							{
								const DtVec3f vert = shape->normalList[ indices[i] ];
								// Set the vertex value.
								//
								for( int j = 0; j < 3; j++)
								{
									vList->vec[j] = vert.vec[j];
								}
								vList++;

								vertexList[ indices[i] ] = 1;
								(*count)++;
							}
						}
					}

					// Free allocated list.
					//
					if( vertexList )
					{
						free( vertexList );
					}
					if( *count > normalCnt ) 
					{
						DtExt_Err("DtGroupGetNormals: error in reallocating memory\n");
					}
					*vertices = (DtVec3f *)realloc( *vertices, *count * sizeof( DtVec3f ));
				}
				return 1;
			}
		}
    }

    return 0;

}  // DtGroupGetNormals


//  ========== DtGroupGetNormal ==========
//
//  SYNOPSIS
//  Return the vertex for the given index.
//
int  DtGroupGetNormal( int shapeID, int groupID, int index, DtVec3f *vertex )
{
    // Check for error.
    //
    if( (shapeID >= 0) && (shapeID < local->shapeCount) )
    {
		ShapeStruct *shape = &local->shapes[shapeID];

        if( (groupID >= 0) && (groupID < shape->groupCount) )
		{
			if( shape->normalList )
			{
				int  faceCnt = 0;

				// Get the number of vertices in the shape.
				//
				int normalCnt = shape->normalCount;

				// Get the indices for the group.
				//
				FaceListStruct face = shape->normalIdx[groupID];

				// Determine the number of unique vertices in the index list.
				//
				if( (faceCnt = face.count) )
				{
					int   count      = 0;
					int  *indices    = face.list;
					char *vertexList = ( char * )calloc( normalCnt, sizeof( char ) );

					for( int i = 0; i < faceCnt; i++ )
					{
						if( indices[i] != DtEND_OF_FACE )
						{
							if( vertexList[ indices[i] ] == 0 )
							{
								if( count == index )
								{
									const DtVec3f vert = shape->normalList[ indices[i] ];

									// Set the vertex value.
									//
									for( int j = 0; j < 3; j++)
									{
										vertex->vec[j] = vert.vec[j];
									}
									break;
								}

								vertexList[ indices[i] ] = 1;
								count++;
							}
						}
					}

					// Free allocated list.
					//
					if( vertexList )
					{
						free( vertexList );
					}
				}
            return 1;
			}
		}
    }

    return 0;

}  // DtGroupGetNormal


//  ========== dtPolygonGetFaceList ==========
//
//  SYNOPSIS
//	Return the number of polygons in the group.
//
//  Q: why dtPoly... not DtPoly... ? What do the translators use?
//
void dtPolygonGetFaceList( int shapeID, int groupID, int **list, int *count )
{
    *list  = local->shapes[ shapeID ].faceIdx[ groupID ].list;
    *count = local->shapes[ shapeID ].faceIdx[ groupID ].count;
}


//  ========== DtPolygonGetCount ==========
//
//  SYNOPSIS
//	Return the number of polygons in the group.
//
//  PA DT does not handle points or lines. 
//
int  DtPolygonGetCount( int shapeID, int groupID, int *count )
{
    int        ret = 0,
               indexCnt = 0;
    int       *vIdx  = NULL,
               *vnIdx = NULL,
               *vtIdx = NULL;

	*count = 0;
	
    if( local->shapes[shapeID].groupCount > 0 ) 
	{
		// Check to see if it is a face.
		//
		DtFaceGetNormalIndexByShape( shapeID, groupID, &indexCnt, &vnIdx);
		DtFaceGetTextureIndexByShape( shapeID, groupID, &indexCnt, &vtIdx);
		ret    = DtFaceGetIndexByShape( shapeID, groupID, &indexCnt, &vIdx);
		*count = DtFaceCount( indexCnt, vIdx);

		// Lines and points:
#if 0
		// If no faces are in the shape and there was not an error.
		//
		if( (0 == *count) && ( kSuccess == ret ) )
		{
			// Check to see if it is a line.
			//
			DtLineGetNormalIndex( shapeID, groupID, &indexCnt, &vnIdx);
			DtLineGetTextureIndex( shapeID, groupID, &indexCnt, &vtIdx);
			ret    = DtLineGetIndex( shapeID, groupID, &indexCnt, &vIdx);
			*count = DtFaceCount( indexCnt, vIdx);

			// Check to see if it is a point.
			//
			if ( (0 == *count ) && ( kSuccess == ret ) )
			{
				DtPointGetNormalIndex( shapeID, groupID, &indexCnt, &vnIdx);
				DtPointGetTextureIndex( shapeID, groupID, &indexCnt, &vtIdx);
				ret    = DtPointGetIndex( shapeID, groupID, &indexCnt, &vIdx);
				*count = DtFaceCount( indexCnt, vIdx);
			}
		}	
#endif
	}	
	
    // If the count is valid then set up an array of pointers 
    // into the index list.  This will be used in the DtPolygonGetIndices
    // routine for fast lookup of polygons.
    //
    if( ret && *count )
    {	
		// Allocate space for the polygon table - a global variable in this file.
		//
		PolygonIndexTable = ( int **)realloc( PolygonIndexTable, *count * sizeof( int *));

	    int i;
		
	    // Load the polygon table.
	    //
		for( i = 0, indexCnt = 0; i < *count; i++) 
		{
			if( DtExt_Debug() )
			{
	//			cerr << "vIdx[" << indexCnt << "] is " << vIdx[ indexCnt] << endl;
			}
	        PolygonIndexTable[ i ] = &vIdx[ indexCnt];

	        while( DtEND_OF_FACE != vIdx[ indexCnt] )
			{
				indexCnt++;
			}
	        indexCnt++;
		}

	    if( vnIdx )
	    {
	        // Allocate space for the normal table.
	        //
	        NormalIndexTable = ( int **) realloc ( NormalIndexTable, *count * sizeof( int *));

	        // Load the polygon table.
			//
			for( i = 0, indexCnt = 0; i < *count; i++) 
			{
	            NormalIndexTable[ i] = &vnIdx[ indexCnt];

	            while( vnIdx[ indexCnt] != DtEND_OF_FACE)
				{
					indexCnt++;
				}
	            indexCnt++;
			}
	    } 
	    else
	    {
	        if( NormalIndexTable )
	        {
				free( NormalIndexTable);
	    	    NormalIndexTable = NULL;
	        }
	    }

	    if( vtIdx )
	    {
	        // Allocate space for the polygon table.
	        //
	        UVIndexTable = ( int **) realloc ( UVIndexTable, *count * sizeof( int *));

	        // Load the polygon table.
			//
			for( i = 0, indexCnt = 0; i < *count; i++) 
			{
	            UVIndexTable[ i] = &vtIdx[ indexCnt];

	            while( vtIdx[ indexCnt] != DtEND_OF_FACE )
				{
					indexCnt++;
				}
	            indexCnt++;
			}
	    } 	
	    else
	    {
	        if( UVIndexTable )
	        {
				free( UVIndexTable);
	    	    UVIndexTable = NULL;
	        }
	    }
	}
    else
    {
		if( PolygonIndexTable)
		{
			free( PolygonIndexTable);
			PolygonIndexTable = NULL;
		}

		if( NormalIndexTable)
		{
			free( NormalIndexTable);
			NormalIndexTable = NULL;
		}

		if( UVIndexTable)
		{
			free( UVIndexTable);
			UVIndexTable = NULL;
		}
	}
    return ret;

}  // DtPolygonGetCount //



//  ========== DtPolygonGetIndices ==========
//
//  SYNOPSIS
//	Returns a polygon give the index into the list. 
//
int  DtPolygonGetIndices( int index, int *vCount, int **vIdx, int **nIdx, int **tIdx)
{
    // Init pointers.
    //
    *vCount = 0;
    *vIdx   = NULL;
    *nIdx   = NULL;
    *tIdx   = NULL;

    // If the PolygonIndexTable exists then look up the polygon.
    //
    if( PolygonIndexTable )
    {
        *vIdx   = PolygonIndexTable[ index];
        *vCount = DtIndexCount( *vIdx);
    }
    else
    {
		return 0;
    }

    // If the NormalIndexTable exists then look up the polygon.
    //
    if( NormalIndexTable )
    {
        *nIdx = NormalIndexTable[ index];
    }

    // If the UVIndexTable exists then look up the polygon.
    //
    if( UVIndexTable )
    {
        *tIdx = UVIndexTable[ index];
    }

    return (*vCount) ? 1 : 0;

} // DtPolygonGetIndices //


//  ========== DtFaceGetIndexByShape ==========
//
//  SYNOPSIS
//	Return the index list for a face.
//
int  DtFaceGetIndexByShape( int shapeID, int groupID, int *count, int **indices )
{
    // Initialize return values.
    //
    *count   = 0;
    *indices = NULL;

    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		return kFailure;
	}
    if( (groupID < 0) || (groupID >= local->shapes[shapeID].groupCount) )
	{ 
		return kFailure;
	}

    if( local->shapes[shapeID].groupCount == 0 ) 
	{
		return( 1 );
	}

	const int num = local->shapes[shapeID].faceIdx[groupID].count;
	const int *list = local->shapes[shapeID].faceIdx[groupID].list;

	if( DtExt_Debug() )
	{
		printFaceListStruct( &local->shapes[shapeID].faceIdx[groupID] );
	}
    // Return values
    //
    *count   = num;
    *indices = (int *)list;

    return kSuccess;

}  // DtFaceGetIndexByShape //


//  ========== DtFaceGetNormalIndexByShape ==========
//
//  SYNOPSIS
//	Return the index list for the normals of a face.
//
int  DtFaceGetNormalIndexByShape( int shapeID, int groupID, int *count, int **indices)
{
    // Initialize return values.
    //
    *count   = 0;
    *indices = NULL;

    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		return kFailure;
	}
    if( (groupID < 0) || (groupID >= local->shapes[shapeID].groupCount) )
	{ 
		return kFailure;
	}
    if( local->shapes[shapeID].groupCount == 0 ) 
	{
		return( 1 ); // why 1?
	}

    // Get the list of normal indicies.
    //
    const int num = local->shapes[shapeID].normalIdx[groupID].count;
    const int *list = local->shapes[shapeID].normalIdx[groupID].list;

	if( DtExt_Debug() )
	{
		printFaceListStruct( &local->shapes[shapeID].normalIdx[groupID] );
	}

    // kludge, test if only index is -1 (comment from PA DT).
    //
    if( (num == 1) && (list[0] == -1) ) 
	{	
		return kSuccess;
	}

    // Return values.
    //
    *count = num;
    *indices = (int *)list;

    return 1;

}  /* DtFaceGetNormalIndexByShape */


//  ========== DtFaceGetColorIndexByShape ==========
//
//  SYNOPSIS
//  Return the index list for the vertex colors of a face.
//
int  DtFaceGetColorIndexByShape( int shapeID, int groupID, int *count, int **indices)
{   
    // Initialize return values.
    //
    *count   = 0;
    *indices = NULL;
    
    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) )
    {   
        return kFailure;
    }
    if( (groupID < 0) || (groupID >= local->shapes[shapeID].groupCount) )
    {   
        return kFailure;
    }
    if( local->shapes[shapeID].groupCount == 0 )
    {   
        return( 1 ); // why 1?
    }
    
    // Get the list of normal indicies.
    //
    const int num = local->shapes[shapeID].vfColorIdx[groupID].count;
    const int *list = local->shapes[shapeID].vfColorIdx[groupID].list;
    
    if( DtExt_Debug() )
    {   
        printFaceListStruct( &local->shapes[shapeID].vfColorIdx[groupID] );
    }
    
    // kludge, test if only index is -1 (comment from PA DT).
    //
    if( (num == 1) && (list[0] == -1) )
    {   
        return kSuccess;
    }
    
    // Return values.
    //
    *count = num;
    *indices = (int *)list;
    
    return 1;

}  /* DtFaceGetColorIndexByShape */


//  ========== DtFaceGetTextureIndexByShape ==========
//
//  SYNOPSIS
//	Return the index list for the texture of a face.
//

int  DtFaceGetTextureIndexByShape( int shapeID, int groupID, int *count, int **indices)
{
    // Initialize return values.
    //
    *count = 0;
    *indices = NULL;

    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) )
	{
		return kFailure;
	}
    if( (groupID < 0) || (groupID >= local->shapes[shapeID].groupCount) )
	{ 	
		return kFailure;
	}
    if( local->shapes[shapeID].groupCount == 0 ) 
	{
		return( 1 ); // why 1?
	}

    // Get the list of vertex indicies.
    //
    const int num = local->shapes[shapeID].stuvIdx[groupID].count;
    const int *list = local->shapes[shapeID].stuvIdx[groupID].list;

	if( DtExt_Debug() )
	{
		printFaceListStruct( &local->shapes[shapeID].stuvIdx[groupID] );
	}

    // kludge, test if only index is -1 (comment from PA)
    //
    if( (num == 1) && (list[0] == -1) ) 
	{
		return( 1); // why 1?
	}
    // Return values:
    //
    *count = num;
    *indices = (int *)list;

    return kSuccess;

}  /* DtFaceGetTextureIndexByShape */


//  ========== DtFaceGetIndexByGroup ==========
//
//  SYNOPSIS
//  Return the index list for a face relative to the group
//      vertex list.
//
int  DtFaceGetIndexByGroup( int shapeID, int groupID, int *count, int **indices)
{
    // Initialize return values.
    //
    *count   = 0;
    *indices = NULL;

    // Check for error.
    //
    if( (shapeID >= 0) && (shapeID < local->shapeCount) )
    {
		ShapeStruct *shape = &local->shapes[shapeID];

		if( shape->vertexList )
		{
			int listSize = shape->vertexCount;

			if( (groupID >= 0) && (groupID < shape->groupCount) )
			{
				int faceCnt = 0;

				// Get the list of vertex indicies.
				//
				FaceListStruct face = shape->faceIdx[groupID];

				if( (faceCnt = face.count) )
				{
					// Allocate space for the return buffer.
					//
					*indices = (int *)malloc( faceCnt * sizeof( int ));
					
					int *coordIndex = face.list;
					int *vertexList = ( int * ) calloc ( listSize, sizeof( int ));

					// Create a list of indices that are used for the group.
					//
					for( int i = 0; i < faceCnt; i++ )
					{
						if( coordIndex[i] == DtEND_OF_FACE )
						{
							(*indices)[i] = DtEND_OF_FACE;
						}
						else
						{
							if( vertexList[ coordIndex[i] ] == 0 )
							{
								vertexList[ coordIndex[i] ] = *count + 1;
								(*indices)[i] = *count;
								(*count)++;
							}
							else
							{
								(*indices)[i] = vertexList[ coordIndex[i] ] - 1;
							}
						}
					}

					*count = faceCnt;

					// Free the allocated memory.
					//
					if( vertexList )
					{
						free( vertexList );
					}
					return 1;
				}
			}
		}
	}

    return 0;

}  // DtFaceGetIndexByGroup


//  ========== DtFaceGetNormalIndexByGroup ==========
//
//  SYNOPSIS
//  Return the index list for the normals of a face relative to
//      the group vertex list.
//
int  DtFaceGetNormalIndexByGroup( int shapeID, int groupID, int *count, int **indices)
{
    // Initialize return values.
    //
    *count   = 0;
    *indices = NULL;

    // Check for error.
    //
    if( (shapeID >= 0) && (shapeID < local->shapeCount) )
    {
		int listSize = 0;

		ShapeStruct *shape = &local->shapes[shapeID];

		if( shape->normalList )
		{
			listSize = shape->normalCount;

			if( (groupID >= 0) && (groupID < local->shapes[shapeID].groupCount) )
			{
				int faceCnt = 0;

				// Get the list of vertex indicies.
				//
				FaceListStruct face = shape->normalIdx[groupID];

				if( (faceCnt = face.count) > 1 )
				{
					// Allocate space for the return buffer.
					//
					*indices = (int *)malloc( faceCnt * sizeof( int ));

					int *normalIndex = ( int * ) face.list;
					int *vertexList  = ( int * ) calloc ( listSize, sizeof( int ));

					// Create a list of indices that are used for the group.
					//
					for( int i = 0; i < faceCnt; i++ )
					{
						if( normalIndex[i] == DtEND_OF_FACE )
						{
							(*indices)[i] = DtEND_OF_FACE;
						}
						else
						{
							if( vertexList[ normalIndex[i] ] == 0 )
							{
								vertexList[ normalIndex[i] ] = *count + 1;
								(*indices)[i] = *count;
								(*count)++;
							}
							else
							{
								(*indices)[i] = vertexList[ normalIndex[i] ] - 1;
							}
						}
					}

					*count = faceCnt;

					// Free the allocated memory.
					//
					if( vertexList )
					{
						free( vertexList );
					}
				}
				return 1;
			}
		}
	}

    return 0;

}  // DtFaceGetNormalIndexByGroup


//  ========== DtFaceGetTextureIndexByGroup ==========
//
//  SYNOPSIS
//  Return the index list for the texture of a face relative
//      to the group vertex list.
//
int  DtFaceGetTextureIndexByGroup( int shapeID, int groupID, int *count, int **indices)
{
    // Initialize return values.
    //
    *count   = 0;
    *indices = NULL;

    // Check for error.
    //
    if( (shapeID >= 0) && (shapeID < local->shapeCount) )
    {
		int listSize;

		ShapeStruct *shape = &local->shapes[shapeID];

		if( shape->stuvList )
		{
			listSize = shape->stuvCount;

			if( (groupID >= 0) && (groupID < shape->groupCount))
			{
				int faceCnt = 0;

				// Get the list of vertex indicies.
				//
				FaceListStruct face = shape->stuvIdx[groupID];

				if( (faceCnt = face.count) > 1 )
				{
					// Allocate space for the return buffer.
					//
					*indices = (int *) malloc ( faceCnt * sizeof( int ));

					int *textureIndex = ( int * ) face.list;
					int *vertexList   = ( int * ) calloc ( listSize, sizeof( int ));

					// Create a list of indices that are used for the group.
					//
					for( int i = 0; i < faceCnt; i++ )
					{
						if( textureIndex[i] == DtEND_OF_FACE )
						{
							(*indices)[i] = DtEND_OF_FACE;
						}
						else
						{
							if( vertexList[ textureIndex[i] ] == 0 )
							{
								vertexList[ textureIndex[i] ] = *count + 1;
								(*indices)[i] = *count;
								(*count)++;
							}
							else
							{
								(*indices)[i] = vertexList[ textureIndex[i] ] - 1;
							}
						}
					}
					*count = faceCnt;
					// Free the allocated memory.
					//
					if( vertexList )
					{
						free( vertexList );
					}
				}
				return 1;
			}
		}
	}
    return 0;

}  // DtFaceGetTextureIndexByGroup


//  ========== DtFaceGetMaterialIndex ==========
//
//  SYNOPSIS
//  Return the MaterialIndex list for a face.
//
int  DtFaceGetMaterialIndex( int shapeID, int groupID, int *count, int **indices )
{
    int err = 0;

    // Initialize return values.
    //
    *count   = 0;
    *indices = NULL;

    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		err = 1;
	}
    if( (groupID < 0) || (groupID >= local->shapes[shapeID].groupCount) ) 
	{ 
		err = 1;
	}
    if( err ) 
	{
		return(0);
	}
    if( local->shapes[shapeID].groupCount == 0 ) 
	{
		return( 1 );
	}

	DtExt_Msg("DtFaceGetMaterialIndex: is not supported\n");

    return 1;

}  // DtFaceGetMaterialIndex //

/*
 *  ========== DtLineGetIndex ==========
 *
 *  SYNOPSIS
 *	Return the vertex index list for a line.
 */

int  DtLineGetIndex( int shapeID, int groupID, int *count, int **indices )
{
    int	err = 0;

    // Initialize return values.
    //
    *count = 0;
    *indices = NULL;

    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		err = 1;
	}
    if( (groupID < 0) || (groupID >= local->shapes[shapeID].groupCount) ) 
	{
		err = 1;
	}
    if( err ) 
	{
		return(0);
	}
	DtExt_Msg("DtLineGetIndex: is not supported\n");

    return(1);

}  /* DtLineGetIndex */


/*
 *  ========== DtLineGetNormalIndex ==========
 *
 *  SYNOPSIS
 *	Return the index list of normals for a line.
 */

int  DtLineGetNormalIndex( int shapeID, int groupID, int *count, int **indices )
{
    int	err = 0;

    // Initialize return values.
    //
    *count = 0;
    *indices = NULL;

    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		err = 1;
	}
    if( (groupID < 0) || (groupID >= local->shapes[shapeID].groupCount) ) 
	{
		err = 1;
	}
    if( err ) 
	{
		return(0);
	}
    if( local->shapes[shapeID].groupCount == 0 ) 
	{
		return( 1 );
	}
    DtExt_Msg("DtLineGetNormalIndex: is not supported\n");

    return(1);

}  /* DtLineGetNormalIndex */


/*
 *  ========== DtLineGetTextureIndex ==========
 *
 *  SYNOPSIS
 *	Return the index list for a texture of a line.
 */

int  DtLineGetTextureIndex( int shapeID, int groupID, int *count, int **indices )
{
    int err = 0;

    // Initialize return values.
    //
    *count = 0;
    *indices = NULL;

    // Check for error
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		err = 1;
	}
    if( (groupID < 0) || (groupID >= local->shapes[shapeID].groupCount) )
	{
		err = 1;
	}
    if( err) 
	{
		return(0);
	}
    if( local->shapes[shapeID].groupCount == 0 ) 
	{
		return( 1 );
	}
    DtExt_Msg("DtLineGetTextureIndex: is not supported\n");

    return(1);

}  /* DtLineGetTextureIndex */


/*
 *  ========== DtPointGetIndex ==========
 *
 *  SYNOPSIS
 *	Return the vertex index list for a point set.
 */

int  DtPointGetIndex( int shapeID, int groupID, int *count, int **indices )
{
    int	err = 0;

    // Initialize return values.
    //
    *count = 0;
    *indices = NULL;

    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		err = 1;
	}
    if( (groupID < 0) || (groupID >= local->shapes[shapeID].groupCount) ) 
	{
		err = 1;
	}
    if( err ) 
	{	
		return(0);
	}
    if( local->shapes[shapeID].groupCount == 0 ) 
	{
		return( 1 );
	}
    DtExt_Msg("DtPointGetIndex: is not supported\n");

    return(1);

}  /* DtPointGetIndex */


/*
 *  ========== DtPointGetNormalIndex ==========
 *
 *  SYNOPSIS
 *	Return the normals index list for a point set.
 */

int  DtPointGetNormalIndex( int shapeID, int groupID, int *count, int **indices )
{
    int	err = 0;

    // Initialize return values.
    //
    *count = 0;
    *indices = NULL;

    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		err = 1;
	}
    if( (groupID < 0) || (groupID >= local->shapes[shapeID].groupCount) ) 
	{
		err = 1;
	}
    if( err) 
	{
		return(0);
	}
    if( local->shapes[shapeID].groupCount == 0 ) 
	{
		return( 1 );
	}
    DtExt_Msg("DtPointGetNormalIndex: is not supported\n");

    return(1);

}  /* DtPointGetNormalIndex */


/*
 *  ========== DtPointGetTextureIndex ==========
 *
 *  SYNOPSIS
 *	Return the texture vertex index list for a point set.
 */

int  DtPointGetTextureIndex( int shapeID, int groupID, int *count, int **indices )
{
    int	err = 0;

    // Initialize return values.
    //
    *count = 0;
    *indices = NULL;

    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		err = 1;
	}
    if( (groupID < 0) || (groupID >= local->shapes[shapeID].groupCount) ) 
	{
		err = 1;
	}
    if( err ) 
	{
		return(0);
	}
    if( local->shapes[shapeID].groupCount == 0 ) 
	{
		return( 1 );
	}
    DtExt_Msg("DtPointGetTextureIndex: is not supported\n");

    return(1);

}  /* DtPointGetTextureIndex */



//  ========== DtNurbsGetNumCtrlPoints ==========
//
//  SYNOPSIS
//	Returns the number of U & V control points in the nurbs.
//

int  DtNurbsGetNumCtrlPoints( int shapeID, int groupID, int *uNum, int *vNum )
{
    // Initialize return values.
    //
    *uNum = 0;
    *vNum = 0;

    // Check for error.
    //
    int	err = 0;

    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		err = 1;
	}
    if( (groupID < 0) || (groupID >= local->shapes[shapeID].groupCount) ) 
	{
		err = 1;
	}
    if( err ) 
	{
		return(0);
	}
    if( local->shapes[shapeID].groupCount == 0 ) 
	{
		return( 1 );
	}

    DtExt_Msg( "DtNurbsGetNumCtrlPoints: is not supported\n" );

    return(1);

}  // DtNurbsGetNumCtrlPoints //



//  ========== DtNurbsGetIndex ==========
//
//  SYNOPSIS
//	Returns the array of index values for the control points.
//

int  DtNurbsGetIndex( int shapeID, int groupID, int *count, int **indices )
{

    // Initialize return values.
    //
    *count = 0;
    *indices = NULL;


    // Check for error.
    //
    int	  err = 0;

    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		err = 1;
	}
    if ((groupID < 0) || (groupID >= local->shapes[shapeID].groupCount)) err = 1;
    if (err) return(0);
    if ( local->shapes[shapeID].groupCount == 0 ) return( 1 );

    DtExt_Msg( "DtNurbsGetIndex: is not supported\n" );

    return(1);


}  // DtNurbsGetIndex //



//  ========== DtNurbsGetUKnotVector ==========
//
//  SYNOPSIS
//	Returns the U knot vector array.
//

int  DtNurbsGetUKnotVector( int shapeID, int groupID, int *num, float **uKnotVector )
{
    // Initialize return values..
    //
    *num = 0;
    *uKnotVector = NULL;

    // Check for error.
    //
    int	err = 0;

    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		err = 1;
	}
    if( (groupID < 0) || (groupID >= local->shapes[shapeID].groupCount) ) 
	{
		err = 1;
	}
    if( err ) 
	{
		return(0);
	}
    if( local->shapes[shapeID].groupCount == 0 ) 
	{
		return( 1 );
	}
    DtExt_Msg("DtNurbsGetKnotVector: is not supported\n");

    return(1);


}  // DtNurbsGetUKnotVector //



//  ========== DtNurbsGetVKnotVector ==========
//
//  SYNOPSIS
//	Returns the V knot vector array.
//

int  DtNurbsGetVKnotVector( int shapeID, int groupID, int *num, float **vKnotVector )
{
    // Initialize return values..
    //
    *num = 0;
    *vKnotVector = NULL;

    // Check for error.
    //
    int	err = 0;

    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		err = 1;
	}
    if( (groupID < 0) || (groupID >= local->shapes[shapeID].groupCount) ) 
	{
		err = 1;
	}
    if( err ) 
	{
		return(0);
	}
    if( local->shapes[shapeID].groupCount == 0 ) 
	{
		return( 1 );
	}
    DtExt_Msg("DtNurbsGetVKnotVector: is not supported\n");

    return(1);
}  // DtNurbsGetVKnotVector //

//  ========== DtNurbsGetTrimVertices ==========
//
//  SYNOPSIS
//	Returns the vertex array for the trimming curves.
//

int  DtNurbsGetTrimVertices( int shapeID, int groupID, int *count, DtVec2f **vertices )
{
    // Initialize return values..
    //
    *count = 0;
    *vertices = NULL;

    // Check for error.
    //
    int	err = 0;

    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		err = 1;
	}
    if( (groupID < 0) || (groupID >= local->shapes[shapeID].groupCount) ) 
	{
		err = 1;
	}
    if( err ) 
	{
		return(0);
	}
    if( local->shapes[shapeID].groupCount == 0 ) 
	{
		return( 1 );
	}
    DtExt_Msg("DtNurbsGetTrimVertices: is not supported\n");

    return( 1 );

}  // DtNurbsGetTrimVertices //


//  ========== DtNurbsTrimGetCount ==========
//
//  SYNOPSIS
//	Returns the number of trimming curves in this nurbs surface.
//

int  DtNurbsTrimGetCount( int shapeID, int groupID, int *count )
{
    // Initialize return values..
    //
    *count = 0;

    // Check for error.
    //
    int	  err = 0;

    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{	
		err = 1;
	}
    if( (groupID < 0) || (groupID >= local->shapes[shapeID].groupCount) ) 
	{
		err = 1;
	}
    if( err ) 
	{
		return(0);
	}
    if( local->shapes[shapeID].groupCount == 0 ) 
	{
		return( 1 );
	}
    DtExt_Msg("DtNurbsTrimsGetCount: is not supported\n");

    // Return requested data.
    //
    // *count = local->shapes[shapeID].groups[groupID].profileCurvesCount;

    return( 1 );


}  // DtNurbsTrimGetCount //



//  ========== DtNurbsTrimGetIndex ==========
//
//  SYNOPSIS
//	Returns the array of index values for the control points.
//

int  DtNurbsTrimGetIndex( int shapeID, int groupID, int trimID, int *count, int **indices )
{

    // Initialize return values.
    //
    *count = 0;
    *indices = NULL;

    // Check for error.
    //
    int err = 0;

    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		err = 1;
	}
    if( (groupID < 0) || (groupID >= local->shapes[shapeID].groupCount) ) 
	{
		err = 1;
	}
    if( local->shapes[shapeID].groupCount == 0 ) 
	{
		return( 1 );
	}

    // if( (trimID < 0)  || 
	// 	   (trimID >= local->shapes[shapeID].groups[groupID].profileCurvesCount)) 
	// err = 1;

    if( err ) 
	{
		return(0);
	}

    DtExt_Msg("DtNurbsTrimGetIndex: is not supported\n");

    return(1);

}  // DtNurbsTrimGetIndex //


//  ========== DtNurbsTrimGetKnotVector ==========
//
//  SYNOPSIS
//	Returns the knot vector for the trimming curve.
//
int  DtNurbsTrimGetKnotVector( int shapeID, int groupID, int trimID, int *num, float **knotVector )
{

    // Initialize return values.
    //
    *num = 0;
    *knotVector = NULL;


    // Check for error.
    //
    int	err = 0;

    if( (shapeID < 0) || (shapeID >= local->shapeCount) )
	{
		err = 1;
	}
    if( (groupID < 0) || (groupID >= local->shapes[shapeID].groupCount) ) 
	{
		err = 1;
	}
    if( local->shapes[shapeID].groupCount == 0 ) 
	{
		return( 1 );
	}

    // if( (trimID < 0)  || 
	// (trimID >= local->shapes[shapeID].groups[groupID].profileCurvesCount))
	// err = 1;

    if( err ) 
	{
		return(0);
	}

    DtExt_Msg("DtNurbsTrimsGetKnotVector: is not supported\n");

    return(1);

}  // DtNurbsTrimGetKnotVector //



//  ========== DtShapeIsValid ==========
//
//  SYNOPSIS
//	Return 1 if none of the attributes of a shape have changed
//	for the current frame.
//

int  DtShapeIsValid( int shapeID, int valid_bit )
{
    int	state = 0;
	int ret = 0;

    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		return(0);
	}

    // Get the current valid bit state.
    //
    // state = local->shapes[shapeID].valid_bits;

    // Check the requested valid bit.
    //
    switch( valid_bit )
    {
		case DT_SHAPE:
			if( (state & (DT_VALID_BIT_MASK & DT_SHAPE_MATRIX)) &&
				(state & (DT_VALID_BIT_MASK & DT_SHAPE_VISIBLE)) &&
				(state & (DT_VALID_BIT_MASK & DT_SHAPE_VERTICIES)) &&
				(state & (DT_VALID_BIT_MASK & DT_SHAPE_NORMALS)) &&
				(state & (DT_VALID_BIT_MASK & DT_SHAPE_TEXTURE_VERTICIES)) )
			{
				ret = 1;
			}
			else
			{
				ret = 0;
			}
			break;
		case DT_SHAPE_MATRIX:
		case DT_SHAPE_VISIBLE:
		case DT_SHAPE_VERTICIES:
		case DT_SHAPE_NORMALS:
		case DT_SHAPE_TEXTURE_VERTICIES:
			ret = (state & (DT_VALID_BIT_MASK & valid_bit) ? 1 : 0);
			break;
			
		default:
			ret = 0;
    }

    // Return valid state.
    //
    return(ret);

}  // DtShapeIsValid //


//  ========== DtGroupIsValid ==========
//
//  SYNOPSIS
//	Returns 1 of the item is valid, 0 otherwise.
//

int  DtGroupIsValid( int shapeID, int groupID, int valid_bit )
{
    int	state = 0;
	int ret = 0;

    // Check for error.
    //
    if( (shapeID < 0) || (shapeID >= local->shapeCount) ) 
	{
		return(0);
	}
    if( (groupID < 0) || (groupID >= local->shapes[shapeID].groupCount) ) 
	{
		return(0);
	}
    if( local->shapes[shapeID].groupCount == 0 ) 
	{
		return( 1 );
	}

    // Get the current valid bit state.
    //
    // state = local->shapes[shapeID].groups[groupID].valid_bits;

    // Check the requested valid bit.
    //
    switch( valid_bit )
    {
		case DT_FACE:
			if( (state & (DT_VALID_BIT_MASK & DT_FACE_VERTEX_INDEX)) &&
				(state & (DT_VALID_BIT_MASK & DT_FACE_NORMAL_INDEX)) &&
				(state & (DT_VALID_BIT_MASK & DT_FACE_TEXTURE_INDEX)) )
			{
				ret = 1;
			}
			else
			{
				ret = 0;
			}
			break;
		case DT_FACE_VERTEX_INDEX:
		case DT_FACE_NORMAL_INDEX:
		case DT_FACE_TEXTURE_INDEX:
			ret = (state & (DT_VALID_BIT_MASK & valid_bit) ? 1 : 0);
			break;

		//	Lines and Points are not supported in this implementation (PA DT):
		//
#if 0
		case DT_LINE:
			if( (state & (DT_VALID_BIT_MASK & DT_LINE_VERTEX_INDEX)) &&
				(state & (DT_VALID_BIT_MASK & DT_LINE_NORMAL_INDEX)) &&
				(state & (DT_VALID_BIT_MASK & DT_LINE_TEXTURE_INDEX)) )
			{
				ret = 1;
			}
			else
			{
				ret = 0;
			}
			break;

		case DT_LINE_VERTEX_INDEX:
		case DT_LINE_NORMAL_INDEX:
		case DT_LINE_TEXTURE_INDEX:
			ret = (state & (DT_VALID_BIT_MASK & valid_bit) ? 1 : 0);
			break;

		case DT_POINT:
			if( (state & (DT_VALID_BIT_MASK & DT_POINT_VERTEX_INDEX)) &&
				 (state & (DT_VALID_BIT_MASK & DT_POINT_NORMAL_INDEX)) &&
				 (state & (DT_VALID_BIT_MASK & DT_POINT_TEXTURE_INDEX)) )
			{
				ret = 1;
			}
			else
			{
				ret = 0;
			}
			break;

		case DT_POINT_VERTEX_INDEX:
		case DT_POINT_NORMAL_INDEX:
		case DT_POINT_TEXTURE_INDEX:
			ret = (state & (DT_VALID_BIT_MASK & valid_bit) ? 1 : 0);
			break;
#endif
		default:
			ret = 0;
    }

    // Return valid state.
    //
    return ret;

}  // DtGroupIsValid //


//  ========== DtIndexCount ==========
//
//  SYNOPSIS
//     Given an face or line this routine determines the
//     number of vertices in the polygon.
//
int DtIndexCount( int *indices )
{
    int indexCnt = 0;

    while( indices[ indexCnt] != DtEND_OF_FACE )
	{
        indexCnt++;
	}
    return indexCnt;
}


//  ========== DtFaceCount ==========
//
//  SYNOPSIS
//      Given an array of indices, this routine returns
//      the number of faces or lines in the topology list.
//
int DtFaceCount( int indexCnt, int *indices )
{
    int i;
    int faceCnt = 0;

	if( NULL == indices )
	{
		return 0; 
	}
    for( i = 0; i < indexCnt; i++) 
    {
        if( indices[i] == DtEND_OF_FACE )
		{
            faceCnt++;
		}
    }
    return faceCnt;
}

int processSurface( MDagPath &transformPath,
				    MObject transformNode, 
					MObject surfaceNode, 
					MObject shapeNode,
					MObject parentNode )
{
	int shapeAdded = kFailure;

	MDagPath childPath;

	if( DtExt_Debug() )
	{
		cerr << "In processSurface\n";
	}
	// Check to make sure that its not already in here.
	//
	for( int i = 0; i < local->shapeCount; i++ )
	{
		if( transformNode ==  local->shapes[i].transformNode )
		{
			if( DtExt_Debug() )
			{
				printf("Found duplicate obj \"%s\" - not added.\n", 
					   objectName( transformNode ) );
			}
			return kFailure;
		}
	}

	MFnNurbsSurface fnSurface( shapeNode );

	// Tesselation is applied on the shape node.
	// 
	// MTesselationParams tessParms( MTesselationParams::kTriangleCountFormat );
	// tessParms.setTriangleCount( 50 );
	MObject midBodySurface;

    MStatus stat;
    MDagPath dagPath;

    MFnDagNode currentDagNode( shapeNode, &stat );

    stat = currentDagNode.getPath( dagPath );

    MFnDagNode fnDN( dagPath );

	childPath = dagPath;

    // Check the tesselation attributes of the node
    //
    MPlug bPlug;
	MPlug lPlug;
	MPlug dPlug;

	int modeU;
	int modeV;
	int numberU;
	int numberV;

    lPlug = fnDN.findPlug( "modeU" );
    lPlug.getValue( modeU );
        
    lPlug = fnDN.findPlug( "numberU" );
    lPlug.getValue( numberU );
        
    lPlug = fnDN.findPlug( "modeV" );
    lPlug.getValue( modeV );
        
    lPlug = fnDN.findPlug( "numberV" );
    lPlug.getValue( numberV );

	bPlug = fnDN.findPlug( "smoothEdge" );
	bool smoothEdge;
	bPlug.getValue( smoothEdge );

	bPlug = fnDN.findPlug( "useChordHeight" );
	bool useChordHeight;
	bPlug.getValue( useChordHeight );

	bPlug = fnDN.findPlug( "useChordHeightRatio" );
	bool useChordHeightRatio;
	bPlug.getValue( useChordHeightRatio );

	bPlug = fnDN.findPlug( "edgeSwap" );
	bool edgeSwap;
	bPlug.getValue( edgeSwap );

	bPlug = fnDN.findPlug( "useMinScreen" );
	bool useMinScreen;
	bPlug.getValue( useMinScreen );

	dPlug = fnDN.findPlug( "chordHeight" );
	double chordHeight;
	dPlug.getValue( chordHeight );

	dPlug = fnDN.findPlug( "chordHeightRatio" );
	double chordHeightRatio;
	dPlug.getValue( chordHeightRatio );

	dPlug = fnDN.findPlug( "minScreen" );
	double minScreen;
	dPlug.getValue( minScreen );

	if( DtExt_tesselate() == kTESSTRI ) 
	{
		MTesselationParams tessParmsTri( MTesselationParams::kGeneralFormat,
										 MTesselationParams::kTriangles );
	
		switch ( modeU )
		{
		case 1:				// Per Surf # of Isoparms in 3D
			tessParmsTri.setUIsoparmType( MTesselationParams::kSurface3DEquiSpaced);
			break;
		case 2:				// Per Surf # of Isoparms
			tessParmsTri.setUIsoparmType( MTesselationParams::kSurfaceEquiSpaced);      
			break;
		case 3:				// Per Span # of Isoparms
			tessParmsTri.setUIsoparmType( MTesselationParams::kSpanEquiSpaced); 
			break;

		case 4:				// Best Guess Based on Screen Size
                    		// There is a comment that 4 uses mode 2 internally
			tessParmsTri.setUIsoparmType( MTesselationParams::kSurfaceEquiSpaced);      
			break;
		}

		tessParmsTri.setUNumber( numberU );


        switch ( modeV )                 
        {
        case 1:             // Per Surf # of Isoparms in 3D
            tessParmsTri.setVIsoparmType( MTesselationParams::kSurface3DEquiSpaced);        
            break;
        case 2:             // Per Surf # of Isoparms
            tessParmsTri.setVIsoparmType( MTesselationParams::kSurfaceEquiSpaced);          
            break;
        case 3:             // Per Span # of Isoparms
            tessParmsTri.setVIsoparmType( MTesselationParams::kSpanEquiSpaced);
            break;
            
        case 4:             // Best Guess Based on Screen Size
							// There is a comment that 4 uses mode 2 internally
            tessParmsTri.setVIsoparmType( MTesselationParams::kSurfaceEquiSpaced);          
            break;
        }   
	
		tessParmsTri.setVNumber( numberV );

		tessParmsTri.setSubdivisionFlag( MTesselationParams::kUseChordHeightRatio, useChordHeightRatio );
		tessParmsTri.setChordHeightRatio( chordHeightRatio );

		tessParmsTri.setSubdivisionFlag( MTesselationParams::kUseMinScreenSize,useMinScreen );
		tessParmsTri.setMinScreenSize( minScreen, minScreen );

#ifndef MAYA101
		tessParmsTri.setSubdivisionFlag( MTesselationParams::kUseEdgeSmooth, smoothEdge );
#endif

		tessParmsTri.setSubdivisionFlag( MTesselationParams::kUseTriangleEdgeSwapping, edgeSwap );

#ifdef OLDTESSELATE
		midBodySurface = fnSurface.tesselate( tessParmsTri, &stat );
#else
		midBodySurface = fnSurface.tesselate( tessParmsTri, transformNode, &stat );
#endif
    }
    else 
	{
		MTesselationParams tessParmsQuad( MTesselationParams::kGeneralFormat,
										  MTesselationParams::kQuads );

        switch ( modeU )                 
        {
        case 1:             // Per Surf # of Isoparms in 3D
            tessParmsQuad.setUIsoparmType( MTesselationParams::kSurface3DEquiSpaced);        
            break;
        case 2:             // Per Surf # of Isoparms
            tessParmsQuad.setUIsoparmType( MTesselationParams::kSurfaceEquiSpaced);          
            break;
        case 3:             // Per Span # of Isoparms
            tessParmsQuad.setUIsoparmType( MTesselationParams::kSpanEquiSpaced);
            break;
            
        case 4:             // Best Guess Based on Screen Size
                            // There is a comment that 4 uses mode 2 internally
            tessParmsQuad.setUIsoparmType( MTesselationParams::kSurfaceEquiSpaced);          
            break;
        }   
        
        tessParmsQuad.setUNumber( numberU );
        
        
        switch ( modeV )
        {
        case 1:             // Per Surf # of Isoparms in 3D
            tessParmsQuad.setVIsoparmType( MTesselationParams::kSurface3DEquiSpaced);        
            break;
        case 2:             // Per Surf # of Isoparms
            tessParmsQuad.setVIsoparmType( MTesselationParams::kSurfaceEquiSpaced);          
            break;
        case 3:             // Per Span # of Isoparms
            tessParmsQuad.setVIsoparmType( MTesselationParams::kSpanEquiSpaced);
            break;
            
        case 4:             // Best Guess Based on Screen Size
                            // There is a comment that 4 uses mode 2 internally
            tessParmsQuad.setVIsoparmType( MTesselationParams::kSurfaceEquiSpaced);          
            break;
        }   
        
        tessParmsQuad.setVNumber( numberV );

        tessParmsQuad.setSubdivisionFlag( MTesselationParams::kUseChordHeightRatio, useChordHeightRatio );
        tessParmsQuad.setChordHeightRatio( chordHeightRatio );
        
        tessParmsQuad.setSubdivisionFlag( MTesselationParams::kUseMinScreenSize,useMinScreen );
        tessParmsQuad.setMinScreenSize( minScreen, minScreen );
        
#ifndef MAYA101
        tessParmsQuad.setSubdivisionFlag( MTesselationParams::kUseEdgeSmooth, smoothEdge );
#endif

        tessParmsQuad.setSubdivisionFlag( MTesselationParams::kUseTriangleEdgeSwapping, edgeSwap );

#ifdef OLDTESSELATE
		midBodySurface = fnSurface.tesselate( tessParmsQuad, &stat );
#else   
        midBodySurface = fnSurface.tesselate( tessParmsQuad, transformNode, 
																	&stat );
#endif

	}

#ifdef OLDTESSELATE
	if( DtExt_Debug() )
	{
		printf( "Adding tesselated model from shape %s under the parent %s\n",
				objectName( shapeNode ), objectName( transformNode ) );
	}

	// Add to the world with the parent.
	// 
	MGlobal::addToModel( midBodySurface, transformNode );
#endif

	//
	// because of the tesselation we are going to lose the shape that we 
	// started with, so we need to get another reference to the new Mesh 
	// created.
	//
	// This is not tested with instances of NurbSurfaces.
	//
	
	MFnDagNode childDagNode( midBodySurface, &stat );
	stat = childDagNode.getPath( childPath );

	shapeAdded = addTransformMesh( childPath, transformNode, 
							surfaceNode, midBodySurface, parentNode );

    return shapeAdded;
}

int
addTransformSurface( MDagPath &transformPath,
					 MObject transformNode,
					 MObject surfaceNode,
					 MObject shapeNode,
					 MObject parentNode )
{
    // Check to see if the Mesh is visible

    MFnDagNode surfNode( surfaceNode );
    MPlug vPlug = surfNode.findPlug( "visibility" );
    MPlug iPlug = surfNode.findPlug( "intermediateObject" );
    
    bool visible, intermediate;
    vPlug.getValue( visible );
    iPlug.getValue( intermediate );
    
    if ( !visible || intermediate )
    {
        return kFailure;
    }

	if( DtExt_Debug() )
	{
       printf( "Adding Transform + Surface node: %s + %s\n", 
			   objectName( transformNode ), objectName( shapeNode ) );
    }

    return processSurface( transformPath, transformNode, 
								surfaceNode, shapeNode, parentNode );
}

int
addShapeTable( unsigned int pathType,
			   MDagPath &transformPath,
			   MObject transformNode,
			   MObject surfaceNode,
			   MObject shapeNode,
			   MObject parentNode )
{	
	if( DtExt_Debug() )
	{
		cerr << "In addShapeTable\n";
	}
	// Check to make sure that its not already in here.
	//

	for( int i = 0; i < local->shapeCount; i++ )
	{
		if ( transformPath.fullPathName() 
				== local->shapes[i].shapeDagPath->fullPathName() )
		{
			if( DtExt_Debug() )
			{
				printf("Found duplicate obj \"%s\" - not added.\n", 
					   objectName( transformNode ) );
			}
			return kFailure;
		}
	}
	
    local->shapes = (ShapeStruct *)realloc(local->shapes, 
								(1+local->shapeCount)*sizeof(ShapeStruct) );
	memset( &local->shapes[local->shapeCount], 0, sizeof(ShapeStruct) );

    local->shapes[local->shapeCount].transformNode = transformNode;
	local->shapes[local->shapeCount].surfaceNode = surfaceNode;
	local->shapes[local->shapeCount].shapeNodeTess = shapeNode;
	local->shapes[local->shapeCount].pathType = pathType;
	
	local->shapes[local->shapeCount].shapeDagPath = new MDagPath( transformPath );

	
	//
	// Lets find out the immediate parent
	//
	
	MDagPath localParentPath( transformPath );
	MObject localParentNode;
	
	if ( localParentPath.length() <= pathType ) 
	{
		localParentNode = MObject::kNullObj;
	} else {
		localParentPath.pop(pathType);
		localParentNode = localParentPath.node();
	}
	
	if( ! localParentNode.isNull() )
	{
		local->shapes[local->shapeCount].parentDagPath = 
										new MDagPath( localParentPath ); 
	}

    if( DtExt_Debug() )
    {
        printf( "Adding: %s to the shape table - ", 
										objectName( transformNode ) );
		if( ! parentNode.isNull() )
		{
		    printf( "parent is: %s.\n", objectName( localParentNode ) );
		}
		else
		{
			printf( "no parent found.\n" );
		}

		cerr << "DtShapeGetCount is " << DtShapeGetCount() << endl;
	}

	//
	// Check for instances
	//

	if ( transformPath.isInstanced() )
	{
        if ( DtExt_Debug() )
			cerr << local->shapeCount << " is Instance node" << endl;
			
		if ( transformPath.instanceNumber() == 0 )
		{
			local->shapes[local->shapeCount].instance = local->shapeCount;
		}
		else
		{
            int originalID;
			MDagPath origPath;
            MDagPath::getAPathTo( transformPath.node(), origPath );

            local->shapes[local->shapeCount].instance = -1;
            for ( originalID = 0;
                  (originalID < local->shapeCount)
                      &&(local->shapes[local->shapeCount].instance == -1);
                  originalID++ )
			{
                if ( origPath.fullPathName() == 
					local->shapes[originalID].shapeDagPath->fullPathName() )
                    local->shapes[local->shapeCount].instance = originalID;
            }
		}
	} else {
		local->shapes[local->shapeCount].instance = local->shapeCount;
	}
	
	local->shapes[local->shapeCount].texCount = 0;

    if( DtExt_Debug() )
	{
        printf( "INFO: Shape %d ", local->shapeCount );
		printf( " has original data at shape %d\n", local->shapes[local->shapeCount].instance );
	}
	return 1;
}

// This function also constructs the face lists.
//
int 
processShaders( MDagPath &transformPath, 
					MObject surfaceNode, MObject shapeNode )
{
	int maxPolygons = 0;
	
	// for now let the user decide on the vertex ordering

	int		invert = DtExt_Winding();
	int		start, end, by;

	bool	uvError;
	bool	nrmError;
	bool	vtxError;
	bool	cpvError;
	
	MStringArray	currGroups;
	
	MStatus stat;
	MFnDependencyNode fnSurfaceNode( surfaceNode );


	// Get path to the surface node
	
	MFnDagNode dnSurfaceNode( surfaceNode );
	MDagPath  surfaceDagPath;
	dnSurfaceNode.getPath( surfaceDagPath );

	MFnDagNode dagNode( shapeNode, &stat );
    MDagPath shapeDagPath;
    stat = dagNode.getPath( shapeDagPath );

	// Need to check to see if we are doing a nurb surface or Mesh
	// surfaceNode may be either.  If the conversion to a MFnMesh 
	// fails then we know that we have a surface and tesselated MFnMesh
	// the tesselated mesh is not going to have any shaders applied and so
	// we will need to do the long method.  Assume only 1 shader on the surface
	// make up a set with just the one shader engine in it.
	
	MFnMesh fnMidBody( surfaceNode, &stat );

	bool givenMesh = false;
	
	if( MS::kSuccess == stat )
	{
	    maxPolygons = fnMidBody.numPolygons( &stat );
		givenMesh = true;

	} else {
		givenMesh = false;

		stat = fnMidBody.setObject( shapeNode );
		if ( MS::kSuccess == stat )
		{
			maxPolygons = fnMidBody.numPolygons( &stat );
		}

	}
	
	// 
	// initialize some of the internal data
	//

    int numShaders = 0;
    int si = local->shapeCount;
    
    FaceListStruct *facelist  = NULL;
    FaceListStruct *STlist    = NULL;
    FaceListStruct *NormVlist = NULL;
    FaceListStruct *NormPlist = NULL;
	FaceListStruct *vfColorlist = NULL;

    int faceCount = 0;

	unsigned instanceNumber = 0;
	MObjectArray sets;
	MObject set;
	MObjectArray comps;
	MObject comp;
	MObject srcNode;
	bool renderableSetsOnly = true;
	bool foundComponent = false;
	unsigned int sk;

	instanceNumber = transformPath.instanceNumber();

	// This function only works on Meshes, need it to work on surfaces as well

	if ( givenMesh )
	{
		fnMidBody.getConnectedSetsAndMembers( instanceNumber, 
											sets, comps, 
											renderableSetsOnly );
	} else {

    	MSelectionList objects;
		MObjectArray setArray;
		MObject mobj;

    	// Get path to the surface node

	    MFnDagNode dnSurfaceNode2( surfaceNode );
	    MDagPath  surfaceDagPath2;
	    dnSurfaceNode2.getPath( surfaceDagPath2 );

		// Add the path to the selection list to find Associated sets
		
		objects.add( surfaceDagPath2 );
		
    	// Get all of the sets that this object belongs to
    	//
    	MGlobal::getAssociatedSets( objects, setArray );
  
   		// reset the Object Arrays for filling in

		sets.clear();
		comps.clear();

    	// Look for a set that is a "shading group"
    	//
    	for ( unsigned int i=0; i<setArray.length(); i++ )
    	{
    	    mobj = setArray[i];
    	    MFnSet fnSet( mobj );

    	    if ( MFnSet::kRenderableOnly == fnSet.restriction(&stat) )
    	    {
				sets.append( mobj );			
				comps.append( MObject::kNullObj );
    	    }   
    	}   
    

	}


	for ( sk = 0; sk < sets.length(); sk++ )
	{
		set = sets[sk];
		comp = comps[sk];

        MFnSet fnSet( set, &stat );
		if ( !stat ) cerr << "ERROR: MFnSet::MFnSet\n";

        MFnDependencyNode dnSet( set, &stat );
		MObject ssAttr = dnSet.attribute( MString( "surfaceShader" ), &stat );

        MPlug ssPlug( set, ssAttr );

        MPlugArray srcPlugArray;
        ssPlug.connectedTo( srcPlugArray,
                            true, // as destination
                            false // as source
                                  );

		if ( srcPlugArray.length() == 1 )
		{
           srcNode = srcPlugArray[0].node();
		} else if ( srcPlugArray.length() == 0 ) {
			continue;
		} else {
			srcNode = srcPlugArray[0].node();
		}

        MItMeshPolygon piter( shapeDagPath, comp, &stat );

		foundComponent = false;

        if ( stat ) {
            for ( ; !piter.isDone(); piter.next() )
            {
				foundComponent = true;
				break;  		// as soon as we know that component here
            }
        }

        // Find the members in this shading engine.
        //         
        MSelectionList members;
        stat = fnSet.getMembers( members, true ); // flatten is true

        if( DtExt_Debug() )
        {
            if( MS::kSuccess == stat )
            {
                cerr << "Got memebers of the shading engine.\n";
            }
            cerr << "Number of members: " << members.length() << endl;                     
        }   
		
		// Check to see if this is the initialShadingGroup

		if ( !strcmp( "initialShadingGroup", dnSet.name().asChar() ) )
		{
			// If we haven't found any components then normally
			// it would mean that the entire object is in this set
			// But if there are more than just this object, I don't
			// think that I really want to use this.  As I'll force
			// all objects to use this initialShadingGroup.  Something
			// doesn't add up.

			// Put this check in here.  If there are no members of the
			// initialShadingGroup, then can it have any faces to render ?

			if ( members.length() == 0 )
				continue;

			if ( !foundComponent )
			{
				if ( sets.length() > 1 )
					continue;
			}
		}
		 
        // Lets see if we have done this shading group before
		// if not then lets do it now.
                    
        int foundShader = false;
        int	usingShader = 0;
		int currentGroup = 0;

        for ( unsigned int i = 0; i < currGroups.length(); i++ )
        {
           if ( !strcmp(objectName( srcNode ),currGroups[i].asChar() ) ) 
           {    
               foundShader = true;
			   usingShader = i;
               break;
           }   
		}       
                        
		int faceIndex = 0;
		int stIndex = 0;
		int normvIndex = 0;
		int normpIndex = 0;
		int vfColorIndex = 0;

        if ( foundShader )
		{	
			currentGroup = usingShader;

            if( DtExt_Debug() )
            {
                printf("using shader \"%s\" with original object \"%s\"\n",
                         objectName( srcNode ), objectName( surfaceNode ) );
            }            

		} 
		else 
        {
            currGroups.append( objectName( srcNode ) );

	        local->shapes[si].groupStructs = 
               (GroupStruct *)realloc(local->shapes[si].groupStructs, 
                    		(1+numShaders)*sizeof(GroupStruct)); 
			memset( &local->shapes[si].groupStructs[numShaders], 0, 
												sizeof(GroupStruct) );  
   	    	facelist = (FaceListStruct *)realloc(facelist,
								(1+numShaders)*sizeof(FaceListStruct)); 
        	facelist[numShaders].count = 0;
        	facelist[numShaders].list = NULL;
        	faceIndex = 0;
                    
        	STlist = (FaceListStruct *)realloc(STlist, 
								(1+numShaders)*sizeof(FaceListStruct));   
        	STlist[numShaders].count = 0;
        	STlist[numShaders].list = NULL;
        	stIndex = 0;
                    
        	NormVlist = (FaceListStruct *)realloc(NormVlist, 
								(1+numShaders)*sizeof(FaceListStruct));
        	NormVlist[numShaders].count = 0;
        	NormVlist[numShaders].list = NULL;
        	normvIndex = 0;
                    
        	NormPlist = (FaceListStruct *)realloc(NormPlist, 
								(1+numShaders)*sizeof(FaceListStruct));
        	NormPlist[numShaders].count = 0;
        	NormPlist[numShaders].list = NULL;
        	normpIndex = 0;
                    
            vfColorlist = (FaceListStruct *)realloc(vfColorlist,
                                (1+numShaders)*sizeof(FaceListStruct));
            vfColorlist[numShaders].count = 0;
            vfColorlist[numShaders].list = NULL;
            vfColorIndex = 0;

        	// Shaders are added to the table.
        	//
        	local->shapes[si].groupStructs[numShaders].shader = srcNode;
        	local->shapes[si].groupStructs[numShaders].surfaceNode = surfaceNode;           
        	currentGroup = numShaders;

	        if( DtExt_Debug() )
	        {
	            printf("Add shader \"%s\" with original object \"%s\"\n",
                         objectName( srcNode ), objectName( surfaceNode ) );
	        }          
   		}

        //MItMeshPolygon faceIter( surfaceNode, &stat );
        MItMeshPolygon faceIter( transformPath, comp, &stat );

        if( MS::kSuccess != stat )
        {
            // cerr << "Error: can not get faceIter\n";
        }   

        if( false  == foundComponent )
        {   
            // This shader is applied on the whole object.
            //
            if( DtExt_Debug() )
            {
                cerr << "shader is applied on the whole object\n";
            }   
            // stat = faceIter.reset( surfaceNode ); 
            // Use the tessellated surfaced.
            //
            stat = faceIter.reset( shapeNode );
        }   
                    

		// start to fill in the data

        if ( faceCount >= maxPolygons )
        {
            if ( DtExt_Debug() )
			{
                 cerr << "we have done all of the faces faceCount = ";
				 cerr << faceCount << " of maxPolygons " << maxPolygons << endl;
            }
			break;
        }   
                    
		uvError = false;
	    nrmError = false;
		vtxError = false;
		cpvError = false;

        for( ; !faceIter.isDone(); faceIter.next() )
        {
          	int numVertices = faceIter.polygonVertexCount( &stat );
            if( DtExt_Debug() > 1 )
            {
                cerr << "Number of vertices in this face is "
                     << numVertices << endl;
                cerr << "This face is made of vertex: ";
            }   
            faceIndex = facelist[currentGroup].count;
            facelist[currentGroup].list = 
            (int * ) realloc(facelist[currentGroup].list,
                            (faceIndex+numVertices+1)*sizeof(int) );
                                     
            stIndex = STlist[currentGroup].count;
            STlist[currentGroup].list = 
                  (int * ) realloc(STlist[currentGroup].list,
                              (stIndex+numVertices+1)*sizeof(int) );
                                     
            normvIndex = NormVlist[currentGroup].count;
            NormVlist[currentGroup].list = 
              		(int * ) realloc(NormVlist[currentGroup].list,
                                 (normvIndex+numVertices+1)*sizeof(int) );
                                     
            normpIndex = NormPlist[currentGroup].count;
            NormPlist[currentGroup].list = 
                    (int * ) realloc(NormPlist[currentGroup].list,
               	                 (normpIndex+numVertices+1)*sizeof(int) );

            vfColorIndex = vfColorlist[currentGroup].count;
            vfColorlist[currentGroup].list = 
            (int * ) realloc(vfColorlist[currentGroup].list,
                            (vfColorIndex+numVertices+1)*sizeof(int) );

            // Lets check to see if we need to invert the winding 
            // order

            if ( invert && !DtShapeIsDoubleSided( si ) &&
                                    DtShapeIsOpposite( si ) )
            {
                start = numVertices - 1;
                end = -1;
                by = -1;
            } else {
                start = 0;
                end = numVertices;
                by = 1;
            }

			int curFaceIndex = faceIter.index();

            for( int q = start; q != end; q += by )
            {
                facelist[currentGroup].count++;
                STlist[currentGroup].count++;
                NormVlist[currentGroup].count++;
                NormPlist[currentGroup].count++;
				vfColorlist[currentGroup].count++;

                int vertexIndex = faceIter.vertexIndex( q, &stat );
                MStatus stat2;
                int normIndex = faceIter.normalIndex( q, &stat2 );
                int uvIndex = 0;
                MStatus uvStat = faceIter.getUVIndex( q, uvIndex );

				int colorIndex = 0;
				MStatus cpvStat = fnMidBody.getFaceVertexColorIndex( curFaceIndex, 
															q, colorIndex );


                if( MS::kSuccess != stat && !vtxError )
                {
                    if ( DtExt_Debug() )
                       cerr << "can not get vertex index\n";
					vtxError = true;
                }
            	if( MS::kSuccess != stat2  && !nrmError )
                {
                   if ( DtExt_Debug() )
                       cerr << "can not get nrm index\n";
					nrmError = true;
                }
                if( MS::kSuccess != uvStat && !uvError)
                {
                    if ( DtExt_Debug() )
                      cerr << "can not get uv index\n";
					uvError = true;
                }

				if( MS::kSuccess != cpvStat && !cpvError )
				{
					if ( DtExt_Debug() )
						cerr << "can not get cpv index=n";
					cpvError = true;
				}

                facelist[currentGroup].list[faceIndex] = vertexIndex;
                STlist[currentGroup].list[stIndex] = uvIndex;
                NormVlist[currentGroup].list[normvIndex] = normIndex;
                NormPlist[currentGroup].list[normpIndex] = curFaceIndex;
				vfColorlist[currentGroup].list[vfColorIndex] = colorIndex;

                faceIndex++;
                stIndex++;
                normvIndex++;
                normpIndex++;
				vfColorIndex++;
           }

           // Close up the current polygon.
           // 
           facelist[currentGroup].count++;
           STlist[currentGroup].count++;
           NormVlist[currentGroup].count++;
           NormPlist[currentGroup].count++;
		   vfColorlist[currentGroup].count++;

           facelist[currentGroup].list[faceIndex] = DtEND_OF_FACE;
           STlist[currentGroup].list[stIndex] = DtEND_OF_FACE;
           NormVlist[currentGroup].list[normvIndex] = DtEND_OF_FACE;
           NormPlist[currentGroup].list[normpIndex] = DtEND_OF_FACE;
		   vfColorlist[currentGroup].list[vfColorIndex] = DtEND_OF_FACE;

           faceIndex++;
           stIndex++;
           normvIndex++;
           normpIndex++;
		   vfColorIndex++;

           faceCount++;
		}

		// Now see if there really was a new shader or if we had a
		// 2nd connection to the same one.

		if ( !foundShader )
			numShaders++;

	}


	local->shapes[si].faceIdx = facelist;
	local->shapes[si].stuvIdx = STlist;
	local->shapes[si].normalIdx = NormVlist;
	local->shapes[si].normPIdx = NormPlist;
	local->shapes[si].vfColorIdx = vfColorlist;
				
	return numShaders;
}

int
addGroupTable( MDagPath &transformPath,
				MObject surfaceNode, MObject shapeNode )
{	
	// MStatus stat = MS::kSuccess;
	// int	status = 0;

	// In processShader, the shader is added to the group  
	// it belongs.
	//
	int numShaders = processShaders( transformPath, surfaceNode, shapeNode );

	int si = local->shapeCount;
	local->shapes[si].shapeNode = surfaceNode;
	local->shapes[si].groupCount = numShaders;
	
	if( DtExt_Debug() )
	{
		printf( "For Shape %d, added %d Groups for \"%s\"\n", 
							si, numShaders, objectName( surfaceNode ) );
	}

	return kSuccess;
}

int
addTransformMesh( MDagPath &transformPath, 
				  MObject transformNode,
				  MObject surfaceNode,
				  MObject shapeNode,
				  MObject parentNode )
{	

	// Check to see if the Mesh is visible

    MFnDagNode surfNode( surfaceNode );
    MPlug vPlug = surfNode.findPlug( "visibility" );
    MPlug iPlug = surfNode.findPlug( "intermediateObject" );

    bool visible, intermediate;
    vPlug.getValue( visible );
    iPlug.getValue( intermediate );

    if ( !visible || intermediate )
	{ 
		return kFailure;
	}

	// MStatus stat = MS::kSuccess;
    
	if( kFailure == addShapeTable( 2, transformPath, transformNode,
								surfaceNode, shapeNode, parentNode ) )
	{
		return kFailure;
	}
	
	// Function processShaders is called from addGroupTable.	
	//
	if( kFailure == addGroupTable( transformPath, surfaceNode, shapeNode ) ) 
	{
		return kFailure;
	}
    
	int result = processMesh( transformNode, surfaceNode, shapeNode );

	// int result = kSuccess;

	if( kSuccess == result )
	{
		local->shapeCount++;
	}
	
	if( DtExt_Debug() )
	{
		cerr << "DtShapeGetCount is " << DtShapeGetCount() << endl;
	}
    return result;
}




int
processMesh( MObject transformNode, MObject surfaceNode, MObject shapeNode )
{
    // Blind Data not implemented at the moment (from PA DT):
    //
    // long    bD_size;
    // char *  bD_data;


	MStatus stat = MS::kSuccess;

    MFnDagNode surfNode( surfaceNode, &stat );
	// MString surfName = surfNode.name( &stat );

	MFnDagNode dagNode( shapeNode, &stat );
	if( MS::kSuccess != stat )
	{
		cerr << "Error: Can not get the FnDagNode for the shape node\n";
	}

    // Take the first dag path.
    //
    MDagPath ShapedagPath;
    stat = dagNode.getPath( ShapedagPath );

	// Now get the transform DagPath

	MFnDagNode transformDagNode( transformNode, &stat );
	if( MS::kSuccess != stat )
	{
		cerr << "Error: Can not get the FnDagNode for the transform node\n";
	}
	MDagPath transformDagPath;
	// may use getAllPath...
	//
	// stat = transformDagNode.getPath( transformDagPath ); 
	stat = dagNode.getPath( transformDagPath ); 

	// Get the lowest transform in the DAG path:
	//
	MObject lowest = transformDagPath.transform();
	if( DtExt_Debug() )
	{
		cerr << "The lowest dag node in the path is " 
							<< objectName( lowest ) << endl;
	}
	// stat = transformDagPath.extendToShape();
	// if( MS::kSuccess != stat )
	// {
	// cerr << "Can not extend the path to shape\n";
	// }

	// MString nodeName = dagNode.name( &stat );

    // AlRenderInfo renderInfo;
	// Assume wire frame for now.
	//


    int     totalNumCoordinates = 0;
    int     totalNumNorms = 0;
    int     totalNumTex = 0;
    float * AlTex = NULL;
	float * AlNorms = NULL;
	float * AlCoords = NULL;
	DtRGBA * vertColor = NULL;
	DtFltRGBA *vertFaceColor = NULL;

	// double  r = 0.0;
	// double  g = 0.0;
	// double  b = 0.0;
	// double  a = 0.0;

    // Get the render info: Mark has put down on his list.
    //
    // polyset->renderInfo( renderInfo );

	//int numShaders = local->shapes[local->shapeCount].groupCount;

    // Retrieve the current TM, and create the inverse transpose.
    //
	// For getting world normal vectors:


	// smooth shading: normal at vertex.
	// flat shading: normal at face.
	// 
	// kTRANSFORMNONE - world - applying matrix to get the world normal.
	//

	MPlug  smoPlug = dagNode.findPlug( "smo" );
	bool  smoothShading;
	smoPlug.getValue( smoothShading );

    MFnMesh fnPoly;
    MFloatVectorArray nArray;
    MVector normal;
	MSpace::Space space;


	// If we are doing world space, then we need to initialize the 
	// Mesh with a DagPath and not just the node

	if ( kTRANSFORMNONE == DtExt_outputTransforms() )
	{
		fnPoly.setObject( ShapedagPath );
		space = MSpace::kWorld;
	} else {
    	fnPoly.setObject( shapeNode );
		space = MSpace::kObject;
	}

	// Now to do the Normals

	if( smoothShading ) {

		totalNumNorms = fnPoly.numNormals( &stat );

		if ( totalNumNorms ) {

			AlNorms = new float[ totalNumNorms * 3 ];

    		fnPoly.getNormals( nArray, space );

			for ( int hh = 0; hh < totalNumNorms; hh++ ) {
				AlNorms[ (hh * 3) ]    = nArray[hh][0];
                AlNorms[ (hh * 3) + 1] = nArray[hh][1];
                AlNorms[ (hh * 3) + 2] = nArray[hh][2];  
			}
		}
	}
	else {

        totalNumNorms = fnPoly.numPolygons( &stat );

        AlNorms = new float[ totalNumNorms * 3 ];

        for ( int hh = 0; hh < totalNumNorms; hh++ )
        {
			fnPoly.getPolygonNormal( hh, normal, space );

            AlNorms[ (hh*3)   ] = normal[0];
			AlNorms[ (hh*3)+1 ] = normal[1];
			AlNorms[ (hh*3)+2 ] = normal[2];
		}
	}

 

    // Now lets define the texture coordinate array.
    //
	// Each vertex has a texture coordinate.
	// Need API functions for getting the uv for each vertex.
	// Work on this when the texture information can be retrieved.
	//

	totalNumTex = fnPoly.numUVs( &stat );

    MFloatArray us;
    MFloatArray vs;

	if ( totalNumTex ) {

		AlTex = new float[ totalNumTex * 2 ];

	    fnPoly.getUVs( us, vs );

		for ( int hh = 0; hh < totalNumTex; hh++ ) {

			AlTex[ (hh * 2) ] = us[hh];
			AlTex[ (hh * 2) + 1] = vs[hh];

		}
	}


    MFloatPointArray pointArray;
	MColorArray colorArray;
	
	totalNumCoordinates = fnPoly.numVertices( &stat );

	if ( totalNumCoordinates ) {

		AlCoords  = new float[ totalNumCoordinates * 3 ];
		vertColor = new DtRGBA [totalNumCoordinates];

		fnPoly.getPoints( pointArray, space );
	
		fnPoly.getVertexColors( colorArray );
		bool colorsOK = false;

		if ( colorArray.length() != (unsigned int)totalNumCoordinates )
		{
			if( DtExt_Debug() )
			{
				printf( "mismatch in color vertices, got %u expected %d\n",
							colorArray.length(), totalNumCoordinates );
			}
		} else {
			colorsOK = true;
		}
		
		for ( int hh = 0; hh < totalNumCoordinates; hh++ ) {

			AlCoords[ (hh * 3) ]    = pointArray[hh][0];
			AlCoords[ (hh * 3) + 1] = pointArray[hh][1];
			AlCoords[ (hh * 3) + 2] = pointArray[hh][2];

			if ( colorsOK )
			{
				if ( colorArray[hh].r == -1 &&
					 colorArray[hh].g == -1 &&
					 colorArray[hh].b == -1  ) 
				{
					vertColor[ hh ].r = 0;
					vertColor[ hh ].g = 0;
					vertColor[ hh ].b = 0;
					vertColor[ hh ].a = 0;

				} else if (  colorArray[hh].r == 0 &&
                		     colorArray[hh].g == 0 &&
                    		 colorArray[hh].b == 0  )
				{
                    vertColor[ hh ].r = 0;
                    vertColor[ hh ].g = 0;
                    vertColor[ hh ].b = 0;
                    vertColor[ hh ].a = 0;

				} else {
                    vertColor[ hh ].r
						= static_cast<unsigned char>(255 * colorArray[hh].r);
                    vertColor[ hh ].g
						= static_cast<unsigned char>(255 * colorArray[hh].g);
                    vertColor[ hh ].b
						= static_cast<unsigned char>(255 * colorArray[hh].b);
                    vertColor[ hh ].a = 1;
				}
			}
		}
	}


	// 
	// Now lets get the color per vertex per face information
	//

	// Lets reset the array to get new version of data

	colorArray.clear();
	fnPoly.getFaceVertexColors( colorArray );	

	int numVertexFaceColors = colorArray.length();

	if ( numVertexFaceColors > 0 )
	{
		vertFaceColor = new DtFltRGBA [numVertexFaceColors];

        for ( int hh = 0; hh < numVertexFaceColors; hh++ ) {
            
            if ( colorArray[hh].r == -1 &&
                 colorArray[hh].g == -1 &&
                 colorArray[hh].b == -1  )
            {   
                vertFaceColor[ hh ].r = 0;
                vertFaceColor[ hh ].g = 0;
                vertFaceColor[ hh ].b = 0;
                vertFaceColor[ hh ].a = 0;
                
            } else {
                vertFaceColor[ hh ].r = colorArray[hh].r;
                vertFaceColor[ hh ].g = colorArray[hh].g;
                vertFaceColor[ hh ].b = colorArray[hh].b;
                vertFaceColor[ hh ].a = 1;
            }
		}
	}	



	// These lists are constructed in processing shaders.
	//
#if 0
	int	usedShaderLists = local->shapes[local->shapeCount].groupCount;

    // Now allocate memory for face lists.
    //
    FaceListStruct * facelist  = new FaceListStruct [ usedShaderLists ];
    FaceListStruct * STlist    = new FaceListStruct [ usedShaderLists ];
	FaceListStruct * NormVlist = new FaceListStruct [ usedShaderLists ];
	FaceListStruct * NormPlist = new FaceListStruct [ usedShaderLists ];

    for( int i = 0; i < usedShaderLists; i++ )
    {
        facelist[i].count = 0;
        facelist[i].list  = NULL;
        STlist[i].count   = 0;
        STlist[i].list    = NULL;
      	NormVlist[i].count = 0;
       	NormVlist[i].list  = NULL;
      	NormPlist[i].count = 0;
       	NormPlist[i].list  = NULL;
    }

    int faceIndex = 0;
    int stIndex   = 0;
    int nrmIndex  = 0;
    int polyIndex = 0;

	for( ; !faceIter.isDone(); faceIter.next() )
	{
		// Can not get the face information from Maya API yet.
		// 
        // AlPolygon * polygon = polyset->polygon( ii );
        // int numVertices = number of vertices per face. Assumed now to be 4.
		// int numVertices = 4;
		int numVertices = faceIter.facetVertexCount( &stat );
		// cerr << "number of vertices in this face is " << numVertices << endl;
		
        int start, end, by;

		// printDtPrivate( local );

        // Get the group number from the current polygon.
		//       
		// int cur_group = polygon->shaderIndex();    // for now use shader index (PA)
		int cur_group = 0;
		cur_group = local->shapes[local->shapeCount].usedGroups[cur_group].count;
		// cerr << "cur_group is " << cur_group << " (has to be 0)\n";

        faceIndex = facelist[cur_group].count;
		stIndex   = STlist[cur_group].count;
		
		if( smoothShading ) 
		{
			nrmIndex = NormVlist[cur_group].count;
		} 
		else 
		{
			polyIndex = NormPlist[cur_group].count;
		}

        facelist[cur_group].list = (int *)realloc( facelist[cur_group].list, 
												   (faceIndex+numVertices+1)*sizeof(int) );
        STlist[cur_group].list   = (int *)realloc( STlist[cur_group].list, 
												   (faceIndex+numVertices+1)*sizeof(int) );

		if( smoothShading ) 
		{ 
        	NormVlist[cur_group].list = (int *)realloc( NormVlist[cur_group].list,
														(faceIndex+numVertices+1)*sizeof(int) );
		} 
		else 
		{
        	NormPlist[cur_group].list = (int *)realloc( NormPlist[cur_group].list,
														(polyIndex+1)*sizeof(int) );
		}

        if( invert ) 
		{
            start = 0;
            end = numVertices;
            by = 1;
        }
        else 
		{
            start = numVertices - 1;
            end = -1;
            by = -1;
        }

        for( int jj = start; jj != end; jj += by )
        {
            facelist[cur_group].count++;
            // facelist[cur_group].list[faceIndex] = polygon->vertexPolysetIndex(jj);
			// cerr << "vertex index in this face is " << jj << endl;
			int vertexIndex = faceIter.vertexIndex( jj, &stat );
			if( MS::kSuccess == stat )
			{	
				// cerr << "for this face, vertex index in the vertex list is " << vertexIndex << endl;
				facelist[cur_group].list[faceIndex] = vertexIndex;
			}
			else
			{
				// cerr << "error in getting the vertex index in the vertex list\n";
			}
#if 0
            STlist[cur_group].count++;
            STlist[cur_group].list[faceIndex] =
				vertData->vertexSTIndex(ii, jj);
#endif

#if 0
			if( smoothShading ) 
			{
            	NormVlist[cur_group].count++;
            	NormVlist[cur_group].list[faceIndex] = vertData->vertexNormalIndex(ii, jj);
				nrmIndex++;
			}
#endif
            faceIndex++;
			stIndex++;
        }

        // If per poly normal, add it here.
        //
#if 0
        if( !smoothShading ) 
		{
			NormPlist[cur_group].count++;
			NormPlist[cur_group].list[polyIndex] = ii; // this ii has to change to some count
        }
#endif 
        // Close up the current polygon.
        //
        facelist[cur_group].count++;
        facelist[cur_group].list[faceIndex] = DtEND_OF_FACE;

        STlist[cur_group].count++;
        STlist[cur_group].list[faceIndex]   = DtEND_OF_FACE;

		if( 1 == smoothShading ) 
		{
        	NormVlist[cur_group].count++;
        	NormVlist[cur_group].list[faceIndex] = DtEND_OF_FACE;
			nrmIndex++;
		}
        faceIndex++;
		stIndex++;
    }
#endif

	DtExt_Msg( "Finished adding polys\n" );

	// cerr << "face list: \n";
	// printFaceListStruct( &facelist[0] );

    // Now copy over the created tables
    // and then increment the count of number of complete shapes.
	//
	int si = local->shapeCount;

    local->shapes[si].vertexCount = totalNumCoordinates;
    local->shapes[si].vertexList = (DtVec3f *)AlCoords;

	local->shapes[si].vertexColor = (DtRGBA *)vertColor;

	local->shapes[si].vfColorCount = numVertexFaceColors;
	local->shapes[si].vfColorList = (DtFltRGBA *)vertFaceColor;

    // Insert the texture coordinates
    //
    local->shapes[si].stuvCount = totalNumTex;
    local->shapes[si].stuvList = (DtVec2f *)AlTex;

    // Insert the normals
    //
    if( 1 == smoothShading ) 
	{
    	local->shapes[si].normalCount = totalNumNorms;
    	local->shapes[si].normalList = (DtVec3f *)AlNorms;

        local->shapes[si].normalPCount = 0;
		local->shapes[si].normalPList = NULL;
	} 
	else 
	{
        local->shapes[si].normalPCount = totalNumNorms;
        local->shapes[si].normalPList = (DtVec3f *)AlNorms;

        local->shapes[si].normalCount = 0;
        local->shapes[si].normalList = NULL;
	}

#if 0
    // Insert the face indices.
    //
    local->shapes[si].faceIdx = facelist;
    local->shapes[si].stuvIdx = STlist;
    local->shapes[si].normalIdx = NormVlist;
    local->shapes[si].normPIdx = NormPlist;
#endif

	// local->shapeCount++;

    // End of stuff that needs to be converted.
    //
    // if( NULL != vertData )
    // {
	//  delete vertData;
    // }

    return kSuccess;
}


//
//	Routine to check children for Lights or Cameras
//

//  utility functions that will be usefull to have for our selves
//  and the users for their own use.

int
hasMeshOrSurfaceChild( MObject transformNode )
{
    int         status = false;
    MStatus stat = MS::kSuccess;
   
    MFnDagNode currentDagNode( transformNode, &stat );

    // Get all the child node.
    //
    int childCount = currentDagNode.childCount( &stat );

	if ( childCount )
	{
		for ( int i = 0; i < childCount; i++ )
		{
		    MObject childNode = currentDagNode.child( i, &stat );

			if ( stat != MS::kSuccess )
				continue;

			if ( (0 == strcmp( "mesh", objectType( childNode ) ) ) || 
				 (0 == strcmp( "nurbsSurface", objectType( childNode ) ) ) )
			{
				MFnDagNode childDagNode( childNode );
	            MPlug vPlug = childDagNode.findPlug( "visibility" );
            	MPlug iPlug = childDagNode.findPlug( "intermediateObject" );
            
           		bool visible, intermediate;
           		vPlug.getValue( visible );
            	iPlug.getValue( intermediate );

				if ( visible && !intermediate )
				{
					status = true;
					break;
				}
			}
			else if ( 0 == strcmp( "transform", objectType( childNode ) )
					|| 0 == strcmp( "joint", objectType( childNode ) ) 
					|| 0 == strcmp( "locator", objectType( childNode ) ) )
			{
				if ( hasMeshOrSurfaceChild( childNode ) )
				{
					status = true;
					break;
				}
			}
        }
        
    }   
    return status;
}   

int
addTransformTransform( int pathType, MDagPath &transformPath,
					   MObject transformNode,
					   MObject parentNode )
{
	if( DtExt_Debug() )
	{
		cerr << "In addTransformTransform\n";
	}
	// If we are adding in the transform node then create a shape for it.
	//
	if( kTRANSFORMALL == DtExt_outputTransforms() )
	{
		// Add in new shape table. The surfaceNode is NULL - 
		// no surface under the transformNode.
		//
		// really only want to add this if it is going to lead to a surface 
		// or mesh object at the bottom.
		//
		// unless the user wants to include "joint only" type hierarchies
		// This is a user defined option
		//

		if ( DtExt_JointHierarchy() || hasMeshOrSurfaceChild( transformNode ) )
		{
			if( addShapeTable( pathType, transformPath, transformNode, 
										MObject::kNullObj, 
										MObject::kNullObj, parentNode ) )
			{
				local->shapeCount++;
				return kSuccess;
			}
		}
	}
	return kFailure;
}

int processDagNode( MDagPath &currentPath,
				    MObject currentNode, 
					MObject parentNode )
{
	int i;
	int shapeAdded = kFailure;
	int shapesCnt = local->shapeCount;
	
	MStatus stat = MS::kSuccess;

	MDagPath dagPath;
	MDagPath childPath;
	
	//MFnDagNode currentDagNode( currentNode, &stat );
	MFnDagNode currentDagNode( currentPath, &stat );

	// Get all the child node.
	//
	int childCount = currentDagNode.childCount( &stat );

	if ( childCount == 0 )
	{
		if ( DtExt_JointHierarchy() )
		{
        	if( ( 0 == strcmp( "transform", objectType( currentNode ) )
                || 0 == strcmp( "joint", objectType( currentNode ) ) )  )
        	{       
			    childPath = currentPath;

            	if( DtExt_Debug() )
            	{
            	    printf( "Process transform/joint -> group node\n" );
            	}   
            	shapeAdded = addTransformTransform( 1, childPath, currentNode,
                                                                parentNode );
            	if( DtExt_Debug() )                                 
            	{
            	    cerr << "parentCount is " << DtShapeGetCount() << endl;
            	}   
            
            	if ( shapeAdded == kSuccess )
					return kSuccess;
			}

		} else {
		
			if ( DtExt_Debug() )
			cerr << "Child count is zero, return with error" << endl;

		}
		
		return kFailure;
	}

	// Consider only one of the children, since their types should be the same.
	// 

	// This is no longer true.  It is possible that each child is different or
	// that maybe there is more than 1 surface/mesh
	
	MSelectionList object;
	MString useChild;
	int pathType;
	
	MObject childNode = currentDagNode.child( 0, &stat );

	childPath = currentPath;
	stat = childPath.extendToShape();
	
	if( DtExt_Debug() )
	{
		printf( "Child node:  %s %s\n", 
				childPath.fullPathName().asChar(), objectName( childNode ) );
		cerr << "currentNode is of type: " << objectType( currentNode ) << endl;
		cerr << "childNode 0 is of type: " << objectType( childNode ) << endl;
	}
	const char *childNodeType = objectType( childNode );

	// Lets go over each child to find items of interest
	
	for( i = 1; i < childCount; i++ )
	{
		MObject thisChildNode = currentDagNode.child( i, &stat );
		if( 0 != strcmp( childNodeType, objectType( thisChildNode ) ) )
		{
			if( DtExt_Debug() )
			{
				cerr << "Warning: this child does not have the same type as the first child\n";
				cerr << "childNode " << i << " is of name " << objectName( thisChildNode) 
					 << " and of type " << objectType( thisChildNode ) << endl;
			}
		}
	}

    // Lets go over each child to find items of interest
	// we should be able to stop once we know that we can add in the current
	// node, each of the child will get found in their own time
	
    for( i = 0; i < childCount; i++ )
	{
		childNode = currentDagNode.child( i, &stat );
	
		// Group Nodes:
		//
		// For some reason, this checking is not working any more. 
		// check again after Maya update.
		// if( ( currentNode.hasFn( MFn::kTransform ) ) &&
		// ( childNode.hasFn( MFn::kTransform ) ) )
		if( ( NULL == objectType( currentNode ) ) ||
			( NULL == objectType( childNode ) ) )
		{
			// Usually, this is a leaf node.
			//
			continue;
		}
			
   		if( ( 0 == strcmp( "transform", objectType( currentNode ) )
				|| 0 == strcmp( "joint", objectType( currentNode ) ) ) &&
			( 0 == strcmp( "transform", objectType( childNode ) ) 
				|| 0 == strcmp( "joint", objectType( childNode ) )
				|| 0 == strcmp( "locator", objectType( childNode ) ) ) )
		{
			if( DtExt_Debug() )
			{
				printf( "Process transform + tranform -> group node\n" );
			}

			if ( 0 == strcmp( "locator", objectType( childNode ) ) )
			{
				pathType = 2;
			} else {
				pathType = 1;
			}
			
			shapeAdded = addTransformTransform( pathType, childPath, currentNode, 
																parentNode );
			if( DtExt_Debug() )
			{
				cerr << "shapeCount is " << DtShapeGetCount() << endl;
			}

			// fast way out of this loop

			if ( shapeAdded == kSuccess )
				break;
		}
		// else if( ( currentNode.hasFn( MFn::kTransform ) ) && 
		// 	 ( childNode.hasFn( MFn::kMesh ) ) )
		if( ( 0 == strcmp( "transform", objectType( currentNode ) ) ) &&
			( 0 == strcmp( "mesh", objectType( childNode ) ) ) )
		{
    	    // See if visible
    	    //
    	    stat = currentDagNode.getPath( dagPath );
    	    if ( !areObjectAndParentsVisible( dagPath ) )
    	    {
    	        continue;
    	    }   
	
			if( DtExt_Debug() )
			{
				printf( "Process transform + mesh -> PolysetNode object\n" );
			}
			shapeAdded = addTransformMesh( childPath, currentNode,
										childNode, childNode, parentNode );
			// fast way out of this loop

			if ( shapeAdded == kSuccess )
				break;

		}

		// else if( ( currentNode.hasFn( MFn::kTransform ) ) && 
		// 	 ( childNode.hasFn( MFn::kNurbsSurface ) ) )
		if( ( 0 == strcmp( "transform", objectType( currentNode ) ) ) &&
			( 0 == strcmp( "nurbsSurface", objectType( childNode ) ) ) )
		{
		    // See if visible
	    	//
	    	stat = currentDagNode.getPath( dagPath );
			if ( !areObjectAndParentsVisible( dagPath ) )
			{
				continue;
			}

			if ( DtExt_Debug() )
			{
				printf( "Process transform + nurbs surface -> SurfaceNode object\n" );
			}
			shapeAdded = addTransformSurface( childPath, currentNode, 
										childNode, childNode, parentNode );

            // fast way out of this loop

            if ( shapeAdded == kSuccess )
                break;

		}	
		// else if( ( currentNode.hasFn( MFn::kTransform ) ) &&
		// 	 ( childNode.hasFn( MFn::kLight ) ) )
		if( ( 0 == strcmp( "transform", objectType( currentNode ) ) ) &&
			( ( 0 == strcmp( "ambientLight", objectType( childNode ) ) ) ||
			  ( 0 == strcmp( "pointLight", objectType( childNode ) ) ) ||
			  ( 0 == strcmp( "spotLight", objectType( childNode ) ) ) ||
			  ( 0 == strcmp( "directionalLight", objectType( childNode ) ) ) ) ) 
		{
			if( DtExt_Debug() )
			{
				printf( "Process light\n" );
			}
			addTransformLight( currentNode, childNode );

		}
		// else if( ( currentNode.hasFn( MFn::kTransform ) ) &&
		// 	 ( childNode.hasFn( MFn::kCamera ) ) )
		if( ( 0 == strcmp( "transform", objectType( currentNode ) ) ) &&
			( 0 == strcmp( "camera", objectType( childNode ) ) ) )
		{
			if( DtExt_Debug() )
			{
				printf( "Process camera\n" );
			}
			addTransformCamera( currentNode, childNode );
		}
	}


	// Now lets see if we added any shapes from any children
	
	if ( shapesCnt != local->shapeCount )
		shapeAdded = kSuccess;
	else
		shapeAdded = kFailure;

	// Return flag to see if they is any added

	return shapeAdded;
}

int
update_Polyset( int shapeID, 
				MObject transformNode, 
				MObject meshShapeNode )
{
	MStatus stat = MS::kSuccess;

	MFnDagNode dagNode( meshShapeNode, &stat );

	MDagPath ShapedagPath;
	
	stat = dagNode.getPath( ShapedagPath );

	// MString nodeName = dagNode.name( &stat );

	MFnDagNode transformDagNode( transformNode, &stat );
	if( MS::kSuccess != stat )
	{
		cerr << "Error: Can not get the FnDagNode for the transform node\n";
	}
	MDagPath transformDagPath;
	// may use getAllPath...
	//
	stat = transformDagNode.getPath( transformDagPath ); 

	// Get the lowest transform in the DAG path:
	//
	/* MObject lowest = */ transformDagPath.transform();

	MItMeshPolygon faceIter( meshShapeNode, &stat );
	if( MS::kSuccess != stat )
	{
		cerr << "Failure in getting the face iterator\n";
	}
    // AlRenderInfo renderInfo;
    // int numPolygons = polyset->numberOfPolygons( );

	int numPolygons = faceIter.count();
	
	if( DtExt_Debug() )
	{
		cerr << "numPolygons is " << numPolygons << endl;
	}

    // First, let's define the vertex normal data.
    // Note that we check to see if this is flat or smooth shaded
    // to make sure we get the right normals.
    //

    MFnMesh fnPoly;
    MFloatVectorArray nArray;
    MFloatVector normal;
    MSpace::Space space;
    
    // If we are doing world space, then we need to initialize the 
    // Mesh with a DagPath and not just the node
    
    if ( kTRANSFORMNONE == DtExt_outputTransforms() )
    {   
        fnPoly.setObject( ShapedagPath );
        space = MSpace::kWorld;
    } else {
        fnPoly.setObject( meshShapeNode );
        space = MSpace::kObject;
    }


	int numPNormals = 0;

    if( local->shapes[shapeID].normalList ) {

        int totalNumNorms = fnPoly.numNormals( &stat );

        if( totalNumNorms ) {

            fnPoly.getNormals( nArray, space );

            for ( int hh = 0; hh < totalNumNorms; hh++ ) {
            	local->shapes[shapeID].normalList[hh].vec[0] = nArray[hh][0];
            	local->shapes[shapeID].normalList[hh].vec[1] = nArray[hh][1];
            	local->shapes[shapeID].normalList[hh].vec[2] = nArray[hh][2];
           	}

		}

	} else if ( local->shapes[shapeID].normalPList ) {

	    numPNormals = fnPoly.numPolygons( &stat );
		MVector pnormal;
		
        for ( int hh = 0; hh < numPNormals; hh++ ) 
		{
			fnPoly.getPolygonNormal( hh, pnormal, space );

            local->shapes[shapeID].normalPList[hh].vec[0] = pnormal[0];
            local->shapes[shapeID].normalPList[hh].vec[1] = pnormal[1];
            local->shapes[shapeID].normalPList[hh].vec[2] = pnormal[2];
		}

	} else {

		cerr << "MDtError: No normal pointers" << endl;

	}

    //
    // Now lets define the texture coordinate array
    //

    int totalNumTex = fnPoly.numUVs( &stat );

    MFloatArray us;
    MFloatArray vs;

    // Check that there are indeed some uv values to update.
    // Should also check that there are the same number of uv's as before
    // Bug# 121622

    if ( local->shapes[shapeID].stuvCount &&
         local->shapes[shapeID].stuvCount == totalNumTex ) {

        fnPoly.getUVs( us, vs );

        for ( int hh = 0; hh < totalNumTex; hh++ ) {

           local->shapes[shapeID].stuvList[hh].vec[0] = us[hh];
           local->shapes[shapeID].stuvList[hh].vec[1] = vs[hh];

        }
    }


	//	Here we take the new positions (assuming they are)
	//	and update the cached positions of the vertices.
	// 

    MFloatPointArray pointArray;

    int totalNumCoordinates = fnPoly.numVertices( &stat );

    if ( totalNumCoordinates ) {

    	fnPoly.getPoints( pointArray, space );
     
        for ( int hh = 0; hh < totalNumCoordinates; hh++ ) {

        	local->shapes[shapeID].vertexList[hh].vec[0] = pointArray[hh][0];
        	local->shapes[shapeID].vertexList[hh].vec[1] = pointArray[hh][1];
        	local->shapes[shapeID].vertexList[hh].vec[2] = pointArray[hh][2];
		}
    }

	return 1;
}


//	
//	update of world space for surface nodes is not supported
//	at the moment, because we may actually change the number of
//	polys that would result from re-tessalation
//
//	So for now it more of a place hold for possible future use
//
int 
update_SurfaceNode( int shapeID, MObject transformNode, 
										MObject surfaceNode )
{
    MStatus stat = MS::kSuccess;

    MFnNurbsSurface fnSurface( surfaceNode );

    // Tesselation is applied on the shape node.
    // 
    // MTesselationParams tessParms( MTesselationParams::kTriangleCountFormat );    // tessParms.setTriangleCount( 50 );

    MFnMeshData dataCreator;
	MObject newOutputData = dataCreator.create(&stat);

	// Bug#121622 - rtg crashes.  Seems that the midBodySurface was getting
	// deleted 1st instead of newOutputData.  something about one not being 
	// referenced counted.  Move the definition of midBodySurface.
	MObject midBodySurface;
	

    MDagPath dagPath;

    MFnDagNode currentDagNode( surfaceNode, &stat );

    stat = currentDagNode.getPath( dagPath );

    MFnDagNode fnDN( dagPath );

    // Check the visibility attribute of the node
    //
    MPlug bPlug;
    MPlug lPlug;
    MPlug dPlug;

    int modeU;
    int modeV;
    int numberU;
    int numberV;

    lPlug = fnDN.findPlug( "modeU" );
    lPlug.getValue( modeU );
        
    lPlug = fnDN.findPlug( "numberU" );
    lPlug.getValue( numberU );
        
    lPlug = fnDN.findPlug( "modeV" );
    lPlug.getValue( modeV );
        
    lPlug = fnDN.findPlug( "numberV" );
    lPlug.getValue( numberV );

	if ( modeU != 3 || modeV != 3 )
	{
    	DtExt_Msg("Warning: Updating Vertices from Surfaces for non spans\n" );
		return 0;
	}


    bPlug = fnDN.findPlug( "smoothEdge" );
    bool smoothEdge;
    bPlug.getValue( smoothEdge );
    
    bPlug = fnDN.findPlug( "useChordHeight" );
    bool useChordHeight;
    bPlug.getValue( useChordHeight );
    
    bPlug = fnDN.findPlug( "useChordHeightRatio" );
    bool useChordHeightRatio;
    bPlug.getValue( useChordHeightRatio );
    
    bPlug = fnDN.findPlug( "edgeSwap" );
    bool edgeSwap;
    bPlug.getValue( edgeSwap );
    
    bPlug = fnDN.findPlug( "useMinScreen" );
    bool useMinScreen;
    bPlug.getValue( useMinScreen );
    
    dPlug = fnDN.findPlug( "chordHeight" );
    double chordHeight;
    dPlug.getValue( chordHeight );
    
    dPlug = fnDN.findPlug( "chordHeightRatio" );
    double chordHeightRatio;
    dPlug.getValue( chordHeightRatio );
    
    dPlug = fnDN.findPlug( "minScreen" );
    double minScreen;
    dPlug.getValue( minScreen );

	if( DtExt_tesselate() == kTESSTRI )
    {
        MTesselationParams tessParmsTri( MTesselationParams::kGeneralFormat,
                                         MTesselationParams::kTriangles );

        switch ( modeU )
        {
        case 1:             // Per Surf # of Isoparms in 3D
            tessParmsTri.setUIsoparmType( MTesselationParams::kSurface3DEquiSpaced);
            break;
        case 2:             // Per Surf # of Isoparms
            tessParmsTri.setUIsoparmType( MTesselationParams::kSurfaceEquiSpaced);
            break;
        case 3:             // Per Span # of Isoparms
            tessParmsTri.setUIsoparmType( MTesselationParams::kSpanEquiSpaced);
            break;

        case 4:             // Best Guess Based on Screen Size
                            // There is a comment that 4 uses mode 2 internally
            tessParmsTri.setUIsoparmType( MTesselationParams::kSurfaceEquiSpaced);
            break;
        }

        tessParmsTri.setUNumber( numberU );


        switch ( modeV )
        {
        case 1:             // Per Surf # of Isoparms in 3D
            tessParmsTri.setVIsoparmType( MTesselationParams::kSurface3DEquiSpaced);
            break;
        case 2:             // Per Surf # of Isoparms
            tessParmsTri.setVIsoparmType( MTesselationParams::kSurfaceEquiSpaced);
            break;
        case 3:             // Per Span # of Isoparms
            tessParmsTri.setVIsoparmType( MTesselationParams::kSpanEquiSpaced);
            break;

        case 4:             // Best Guess Based on Screen Size
                            // There is a comment that 4 uses mode 2 internally
            tessParmsTri.setVIsoparmType( MTesselationParams::kSurfaceEquiSpaced);
            break;
        }

        tessParmsTri.setVNumber( numberV );

        tessParmsTri.setSubdivisionFlag( MTesselationParams::kUseChordHeightRatio, useChordHeightRatio );
        tessParmsTri.setChordHeightRatio( chordHeightRatio );

        tessParmsTri.setSubdivisionFlag( MTesselationParams::kUseMinScreenSize,useMinScreen );
        tessParmsTri.setMinScreenSize( minScreen, minScreen );

#ifndef MAYA101
        tessParmsTri.setSubdivisionFlag( MTesselationParams::kUseEdgeSmooth, smoothEdge );
#endif

        tessParmsTri.setSubdivisionFlag( MTesselationParams::kUseTriangleEdgeSwapping, edgeSwap );

#ifdef OLDTESSELATE
        midBodySurface = fnSurface.tesselate( tessParmsTri, &stat );
#else

		// Really I don't want to add this tesselation into the DAG graph
		// it is an itermediate step, so should pass in a dataMesh as the
		// parent and then be able to delete it after.

		midBodySurface = fnSurface.tesselate( tessParmsTri,newOutputData, 
								&stat ); 
#endif
    }
    else
    {
        MTesselationParams tessParmsQuad( MTesselationParams::kGeneralFormat,
                                          MTesselationParams::kQuads );

        switch ( modeU )
        {
        case 1:             // Per Surf # of Isoparms in 3D
            tessParmsQuad.setUIsoparmType( MTesselationParams::kSurface3DEquiSpaced);
            break;
        case 2:             // Per Surf # of Isoparms
            tessParmsQuad.setUIsoparmType( MTesselationParams::kSurfaceEquiSpaced);
            break;
        case 3:             // Per Span # of Isoparms
            tessParmsQuad.setUIsoparmType( MTesselationParams::kSpanEquiSpaced);            break;

        case 4:             // Best Guess Based on Screen Size
                            // There is a comment that 4 uses mode 2 internally
            tessParmsQuad.setUIsoparmType( MTesselationParams::kSurfaceEquiSpaced);
            break;
        }

        tessParmsQuad.setUNumber( numberU );


        switch ( modeV )
        {
        case 1:             // Per Surf # of Isoparms in 3D
            tessParmsQuad.setVIsoparmType( MTesselationParams::kSurface3DEquiSpaced);
            break;
        case 2:             // Per Surf # of Isoparms
            tessParmsQuad.setVIsoparmType( MTesselationParams::kSurfaceEquiSpaced);

            break;
        case 3:             // Per Span # of Isoparms
            tessParmsQuad.setVIsoparmType( MTesselationParams::kSpanEquiSpaced);            break;
            
        case 4:             // Best Guess Based on Screen Size
                            // There is a comment that 4 uses mode 2 internally
            tessParmsQuad.setVIsoparmType( MTesselationParams::kSurfaceEquiSpaced);          
            break;
        }
        
        tessParmsQuad.setVNumber( numberV );
        
        tessParmsQuad.setSubdivisionFlag( MTesselationParams::kUseChordHeightRatio, useChordHeightRatio );
        tessParmsQuad.setChordHeightRatio( chordHeightRatio );
        
        tessParmsQuad.setSubdivisionFlag( MTesselationParams::kUseMinScreenSize,useMinScreen );
        tessParmsQuad.setMinScreenSize( minScreen, minScreen );

#ifndef MAYA101
        tessParmsQuad.setSubdivisionFlag( MTesselationParams::kUseEdgeSmooth, smoothEdge );
#endif      

        tessParmsQuad.setSubdivisionFlag( MTesselationParams::kUseTriangleEdgeSwapping, edgeSwap );         
            
#ifdef OLDTESSELATE
        midBodySurface = fnSurface.tesselate( tessParmsQuad, &stat );
#else   
        
        // Really I don't want to add this tesselation into the DAG graph
        // it is an intermediate step, so should pass in a dataMesh as the
        // parent and then be able to delete it after.
        
        midBodySurface = fnSurface.tesselate( tessParmsQuad, newOutputData,
                                &stat );
#endif

    }       


    //MFnDagNode dagNode( midBodySurface, &stat );

	MItMeshPolygon faceIter( midBodySurface, &stat );
	// int numPolygons = faceIter.count();

	// Here we should check that the new tesselation has the same number 
	// of polygons/vertices as the original tesselation.  If not then we 
	// really probably don't want to continue;


    // First, let's define the vertex normal data.
    // Note that we check to see if this is flat or smooth shaded
    // to make sure we get the right normals.
    //
    
    MFnMesh fnPoly;
    MFloatVectorArray nArray;
    MFloatVector normal;
    MSpace::Space space;
    
    // If we are doing world space, then we need to initialize the 
    // Mesh with a DagPath and not just the node
    
    if ( kTRANSFORMNONE == DtExt_outputTransforms() )
    {   
		// At the moment the world space Surface probably won't work
		// probably need to get the local space, and derive the 
		// world from the transform global matrix.  Do later if needed

        fnPoly.setObject( midBodySurface );
        space = MSpace::kWorld;
    } else {
        fnPoly.setObject( midBodySurface );
        space = MSpace::kObject;
    }
   

	// Lets do one of our checks here

	int newVertexCount = fnPoly.numVertices( &stat );
	
	if ( newVertexCount != local->shapes[shapeID].vertexCount )
	{
       	DtExt_Msg("Warning: different number of vertices when Animating\n" );
		cerr << "Warning: different number of vertices when Animating" 
			 << newVertexCount << " old count " <<
			 local->shapes[shapeID].vertexCount << endl;

		return 0;
	}
		
    int numPNormals = 0;
    
    if( local->shapes[shapeID].normalList ) {
    
        int totalNumNorms = fnPoly.numNormals( &stat );
        
        if( totalNumNorms ) {
        
            fnPoly.getNormals( nArray, space );
            
            for ( int hh = 0; hh < totalNumNorms; hh++ ) {
                local->shapes[shapeID].normalList[hh].vec[0] = nArray[hh][0];
                local->shapes[shapeID].normalList[hh].vec[1] = nArray[hh][1];
                local->shapes[shapeID].normalList[hh].vec[2] = nArray[hh][2];
            }   
            
        }   
        
    } else if ( local->shapes[shapeID].normalPList ) {
    
        numPNormals = fnPoly.numPolygons( &stat );

		if ( numPNormals != local->shapes[shapeID].normalPCount )
		{ 
		   	DtExt_Msg("Warning: different number of normals when Animating\n" );
			cerr << "Warning: different number of vertices when Animating"
					<< newVertexCount << " old count " 
					<< local->shapes[shapeID].vertexCount << endl;

			return 0;
		}

        MVector pnormal;
        
        for ( int hh = 0; hh < numPNormals; hh++ )
        {
            fnPoly.getPolygonNormal( hh, pnormal, space );
            
            local->shapes[shapeID].normalPList[hh].vec[0] = pnormal[0];
            local->shapes[shapeID].normalPList[hh].vec[1] = pnormal[1];
            local->shapes[shapeID].normalPList[hh].vec[2] = pnormal[2];
        }   
        
    } else {

		cerr <<  "MDtErr: No normal pointers found" << endl; 

	}


    //
    // Now lets define the texture coordinate array
    //
        
    int totalNumTex = fnPoly.numUVs( &stat );

    MFloatArray us;
    MFloatArray vs;
        
    // Check that there are indeed some uv values to update.
    // Should also check that there are the same number of uv's as before
    // Bug# 121622

    if ( local->shapes[shapeID].stuvCount &&
         local->shapes[shapeID].stuvCount == totalNumTex ) {

        fnPoly.getUVs( us, vs );
            
        for ( int hh = 0; hh < totalNumTex; hh++ ) {
           local->shapes[shapeID].stuvList[hh].vec[0] = us[hh];
           local->shapes[shapeID].stuvList[hh].vec[1] = vs[hh];
        }   
    }   


    //  Here we take the new positions (assuming they are)
    //  and update the cached positions of the vertices.
    // 
    
        
    MFloatPointArray pointArray;
        
    int totalNumCoordinates = fnPoly.numVertices( &stat );
        
    if ( totalNumCoordinates ) {

        fnPoly.getPoints( pointArray, space );

        for ( int hh = 0; hh < totalNumCoordinates; hh++ ) {

            local->shapes[shapeID].vertexList[hh].vec[0] = pointArray[hh][0];
            local->shapes[shapeID].vertexList[hh].vec[1] = pointArray[hh][1];
            local->shapes[shapeID].vertexList[hh].vec[2] = pointArray[hh][2];
        }
    }

	return 1;
}


//  Used in DtExt_.h
/*
//	Update all of the shapes/groups vertices for the current viewframe
//	Will go thru all of the shapes and find the original polyset/surface
//	and update the points in world/local space.
*/

int
DtExt_SetupWorldVertices( )
{
	MObject obj;

	if( NULL == local )
	{
		DtExt_Err("Error: Trying to setup World vertices before its time.\n" );
		return 0;
	}

	// Allow the user to turn this off if wanted
	// Will make thing go faster if not changing

	if ( !DtExt_VertexAnimation() )
	{
		return 0;
	}

	for( int i = 0; i < local->shapeCount; i++ )
	{
		obj = local->shapes[i].surfaceNode;

		// This I added in, due to an invalid pointer, but I think it was 
		// in another translator that corrupted memory, and just happened 
		// to affect this here.
		// For now I will keep this open
		
		if ( !objectType( obj ))
		{
			continue;
		}		

		if( //obj.hasFn( MFn::kMesh ) )	
			0 == strcmp( "mesh", objectType( obj ) ) )
		{
			int count;
			int *vertices = NULL;

			if ( DtShapeGetVerticesAnimated(i, &count, &vertices))
			{
				update_Polyset( i, local->shapes[i].transformNode, obj );
			}

			if ( vertices )
				delete [] vertices;

		}

		// Well, lets try something for equal # per iso span, seems to
		// be reasonable well behaved
		//
		//
		else if( // obj.hasFn( MFn::kNurbsSurface ) )
			0 == strcmp( "nurbsSurface", objectType( obj ) ) )
		{
			update_SurfaceNode( i, local->shapes[i].transformNode, obj );
		}

	} 
	return 1;
}


/* ====================================================================== *
 * -------------------  PRIVATE FUNCTIONS  ------------------------------ *
 * ====================================================================== */


/*
 *  ========== shapeNew ==========
 *
 *  SYNOPSIS
 *	A private function. Used to reset all internal states.
 */

void  shapeNew( void )
{
    // Create the object instance structure.
    //
    if( NULL == local ) 
	{
		local = (DtPrivate *)calloc( 1, sizeof( DtPrivate ) );
	}

    // Lets go out and do a little parsing of the wire file
    // in order to generate the local database of the Shapes/Groups
    // which is the basis for the Dt API.
    //
	// printDtPrivate( local );

	// Check the return value to see if we were interrupted
	// if so clean up this part and return
	
	if ( traverseDag() == 1 )
		DtExt_ShapeDelete();

	// printDtPrivate( local );

}  // shapeNew //


// 
//	Delete shapes cache
//

void
DtExt_ShapeDelete()
{
	int i, j, k;

	if( NULL == local ) 
	{
		return;
	}

	// If there are any shapes then delete them.
	//
	if( 0 != local->shapeCount )
	{
		for( i = 0; i < local->shapeCount; i++ )
		{
			if( local->shapes[i].groupCount )
			{
				for( j = 0; j < local->shapes[i].groupCount; j++ )
				{
					if ( local->shapes[i].groupStructs[j].mtlName )
					{
						free (local->shapes[i].groupStructs[j].mtlName);
					}

					if ( local->shapes[i].groupStructs[j].textureName )
					{
						free ( local->shapes[i].groupStructs[j].textureName );
					}
				}
			}

			free ( local->shapes[i].groupStructs );

			if ( !local->shapes[i].surfaceNode.isNull() && 
			 					 !local->shapes[i].shapeNodeTess.isNull() )
			{
				//
				//	If the two are not the same then a surface was
				//	tessalated and the polyset added to the Dag Tree
				//	as a Sibling
				//
				//	So to clean it all up, we should delete the node
				//

        		if ( !(local->shapes[i].surfaceNode == 
            			           local->shapes[i].shapeNodeTess) )
        		{
                	MGlobal::removeFromModel( local->shapes[i].shapeNodeTess );
				}
        	}

			
       		if ( local->shapes[i].faceIdx  )
			{
				for (k = 0; k < local->shapes[i].groupCount; k++ )
				{
        	    	free( local->shapes[i].faceIdx[k].list );
				}
				//delete [] local->shapes[i].faceIdx;
				free( local->shapes[i].faceIdx );
        	}
        	if ( local->shapes[i].stuvIdx  )
			{
        	    for (k = 0; k < local->shapes[i].groupCount; k++ )
        	        free( local->shapes[i].stuvIdx[k].list );
        	    //delete [] local->shapes[i].stuvIdx;
				free( local->shapes[i].stuvIdx );
        	}
        	if ( local->shapes[i].normalIdx  )
			{
        	    for (k = 0; k < local->shapes[i].groupCount; k++ )
        	        free( local->shapes[i].normalIdx[k].list );
        	    //delete [] local->shapes[i].normalIdx;
				free( local->shapes[i].normalIdx );
        	}
        	if ( local->shapes[i].normPIdx  )
			{
        	    for (k = 0; k < local->shapes[i].groupCount; k++ )
        	        free( local->shapes[i].normPIdx[k].list );
        	    //delete [] local->shapes[i].normPIdx;
				free( local->shapes[i].normPIdx );
        	}
            if ( local->shapes[i].vfColorIdx  )
            {
                for (k = 0; k < local->shapes[i].groupCount; k++ )
                    free( local->shapes[i].vfColorIdx[k].list );
                //delete [] local->shapes[i].vfColorIdx;
                free( local->shapes[i].vfColorIdx );
            }

            if ( local->shapes[i].usedGroups  )
            {
                delete [] local->shapes[i].usedGroups;
            }


       		if ( local->shapes[i].vertexList  )
			{
			     delete [] local->shapes[i].vertexList;
        	}
        	if ( local->shapes[i].normalList  )
			{
        	    delete [] local->shapes[i].normalList;
        	}
        	if ( local->shapes[i].stuvList  )
			{
        	    delete [] local->shapes[i].stuvList;
        	}
        	if ( local->shapes[i].normalPList  )
			{
        	    delete [] local->shapes[i].normalPList;
        	}
            if ( local->shapes[i].vertexColor  )
            {
                delete [] local->shapes[i].vertexColor;
            }
			if ( local->shapes[i].vfColorList )
			{
				delete [] local->shapes[i].vfColorList;
			}

			if ( local->shapes[i].shapeDagPath )
			{
				delete local->shapes[i].shapeDagPath;
			}

            if ( local->shapes[i].parentDagPath )
            {
                delete local->shapes[i].parentDagPath;
            }

			// Get rid of the Vertex Key Frames if any

			if ( local->shapes[i].vtxAnimKeyFrames )
			{
				delete local->shapes[i].vtxAnimKeyFrames;
			}

			if ( local->shapes[i].trsAnimKeyFrames )
			{
				delete local->shapes[i].trsAnimKeyFrames;
			}
		}
	}

	free ( local->shapes );
	free ( local );
	local = NULL;

}

//  ========== traverseDag ==========
//
//  SYNOPSIS
//	Create the node caches and prepare for exporting.
//
int  traverseDag( void )
{
	MStatus stat = MS::kSuccess;
	MDagPath dagPath;

	int	returnStatus = 0;


	// The whole DAG traversal or selection list traversal.
	//
	MItDag dagIterator( 
		MItDag::kDepthFirst,	// Traversal Type
		//MItDag::kBreadthFirst,	// Traversal Type
		MFn::kInvalid,			// Filter - ie. what the iterator selects
		&stat );

	if( MS::kSuccess != stat )
	{
		DtExt_Err( "Error: Failure in DAG iterator setup.\n" );
		return 0;
	}

	// Scan the entire DAG and output the name and depth of each node.
	//

	MDagPath currentPath;
	
#ifndef MAYA101
	//
	//	if we have this function then lets use it
	//
	//	Allow the user to break out of the process loop early
	//

	MComputation computation;

	computation.beginComputation();
#endif

	MObject lastNode = MObject::kNullObj;
	
	for( ; !dagIterator.isDone(); dagIterator.next() )
	{
		// Retrieve the the current item pointed to by the iterator.
		//
		MObject currentNode = dagIterator.item( &stat );
		stat = dagIterator.getPath( currentPath ); 

		// Create the function set interface to the current DAG node
		// to access DAG node methods.
		//
		//MFnDagNode fnDagNode( currentNode, &stat );
		MFnDagNode fnDagNode( currentPath, &stat );

        if( DtExt_Debug() )
		{
			// Get the depth of the node.
			//
			unsigned int nodeDepth = dagIterator.depth( &stat );
			
			// Output the name and depth of the node.
			// Note that MString::asChar() returns a char*.
			
			printf( "\nCurrent node is: \"%s", objectName( currentNode ) );
			printf( "\" at depth: %u in the DAG.\n", nodeDepth );
		}

		// Before processing this dagNode, lets see if it is needed
		// if we are doing selective or active exports

		if ( PICKED_Nodes == DtExt_WalkMode() )
		{
            stat = fnDagNode.getPath( dagPath );

			if ( !isObjectSelected( dagPath ) )
			{
				lastNode = currentNode;
				continue;
			}
		}

		// Now try the active modes

		else if ( ACTIVE_Nodes == DtExt_WalkMode() )
		{
	        stat = fnDagNode.getPath( dagPath );

			if ( !isObjectOrParentSelected( dagPath ) )
			{
				lastNode = currentNode;
				continue;
			}
		}

		// Get the parent of the node.
		// 
		int parentCount = fnDagNode.parentCount( &stat );
		if( DtExt_Debug() )
		{
			printf( "parentCount is %d\n", parentCount );
		}

		if( 0 == parentCount )
		{
			processDagNode( currentPath, currentNode, MObject::kNullObj );
		}
		else
		{
			// This node has instances if it has more than one parent.
			// We will consider the first parent as the original parent.
			//
			// Add the object to the tables with the first parent. 
			// Deal with the rest later.
			//
			
			MObject firstParentNode = lastNode;
			int shapeAdded = processDagNode( currentPath, currentNode,
														firstParentNode );
			if( DtExt_Debug() )
			{
				cerr << "parent 0: " << objectName( firstParentNode ) << endl;
			}
			if( kFailure == shapeAdded )
			{
				if( DtExt_Debug() )
				{
					cerr << "This object is not added to the shape table\n";
				}
			}
			else 
			{
				if( DtExt_Debug() )
				{
					cerr << "local->shapeCount-1 is " 
									<< local->shapeCount-1 << endl;
					cerr << "shape node is " 
						<< objectName( local->shapes[local->shapeCount-1].shapeNode ) << endl;
				}

			}
		}

#ifndef MAYA101
		if ( computation.isInterruptRequested() )
		{
			returnStatus = 1;
			break;
		}
#endif

		// remember who was last

		lastNode = currentNode;
		
	}

#ifndef MAYA101
	computation.endComputation();

	if ( returnStatus )
		return returnStatus;

#endif


	// Determine the parents of the shape nodes.
	//

	if ( DtExt_Debug() )
	{
		cerr << "Determine parents of shapes nodes" << endl;
	}

	MDagPath parentTest;

	for( int i = 0; i < local->shapeCount; i++ )
    {
        ShapeStruct *shape = &local->shapes[i];

		// Find the parent of the shape.
		//
		if( shape->shapeDagPath )
		{
			if ( DtExt_Debug() )
			{
				cerr << "shape: " << i 
					 << " (" << shape->shapeDagPath->fullPathName().asChar() << ") ";
			}

			// If a parent exists, search the shape list for the parent shape.
			// Else, set the parent to NULL.
			//
			if( shape->parentDagPath )
			{
				if ( DtExt_Debug() )
				{
					cerr << "has parent " << shape->parentDagPath->fullPathName().asChar()
						 << " look in [";
				}

				for( int j = 0; j < local->shapeCount; j++ )
				{
					if ( i == j ) 
						continue;
					

					if ( DtExt_Debug() )
					{
						cerr << local->shapes[j].shapeDagPath->fullPathName().asChar() << ",";
					}
					
					// Set the temp path to be the normal one to check

					parentTest.set( *local->shapes[j].shapeDagPath );

					// See if this is a mesh shape, if so then lets find its transform

					if ( local->shapes[j].pathType == 2 )  // Mesh
					{
						parentTest.pop(1);
					}

					if( shape->parentDagPath->fullPathName() == 
						parentTest.fullPathName() )
					{
						shape->firstParentStruct = &local->shapes[j];
						
						if ( DtExt_Debug() )
						{
							cerr << "]\nfound ";
							cerr << parentTest.fullPathName().asChar();
						}

						break;
					}

				}
			}
			
			if ( DtExt_Debug() )
			{
				cerr << "." << endl;
			}
		}
    }

	return returnStatus;

}  // traverseDag //

#ifdef WIN32
#pragma warning(default: 4244)
#endif // WIN32
