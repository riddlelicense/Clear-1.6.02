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

///////////////////////////////////////////////////////////////////
//
// DESCRIPTION: 
//		This is an example of using the built in texture cache for drawing.
//
///////////////////////////////////////////////////////////////////

#ifdef WIN32
#pragma warning( disable : 4786 )		// Disable STL warnings.
#endif

#include <maya/MIOStream.h>
#include <maya/MString.h>
#include <maya/MPlug.h>
#include <maya/MFnPlugin.h>
#include <maya/MPxHwShaderNode.h>
#include <maya/MFnMesh.h>

#if MAYA_API_VERSION >= 800
	#include <maya/MHwTextureManager.h>
	#include <maya/MImageFileInfo.h>
#endif

#if defined(OSMac_MachO_)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

class hwManagedTextureShader : public MPxHwShaderNode
{
	public:
                    hwManagedTextureShader();
    virtual         ~hwManagedTextureShader();

	virtual void    postConstructor();

	virtual MStatus	glBind(const MDagPath& shapePath);
	virtual MStatus	glUnbind(const MDagPath& shapePath);
	virtual MStatus	glGeometry( const MDagPath&,
                              int prim,
							  unsigned int writable,
							  int indexCount,
							  const unsigned int * indexArray,
							  int vertexCount,
							  const int * vertexIDs,
							  const float * vertexArray,
							  int normalCount,
							  const float ** normalArrays,
							  int colorCount,
							  const float ** colorArrays,
							  int texCoordCount,
							  const float ** texCoordArrays);

	virtual int		texCoordsPerVertex();
	virtual int		normalsPerVertex();
	virtual bool	supportsBatching() const;

    static  void *  creator();
    static  MStatus initialize();

	GLboolean lightingOn;
	bool boundTexture;

    static  MTypeId id;
};

// This ID provided by Maya. Do not change.
MTypeId hwManagedTextureShader::id( 0x81033 );

void hwManagedTextureShader::postConstructor( )
{
	setMPSafe(false);
}

hwManagedTextureShader::hwManagedTextureShader()
{
}

hwManagedTextureShader::~hwManagedTextureShader()
{
}

void * hwManagedTextureShader::creator()
{
    return new hwManagedTextureShader();
}

MStatus hwManagedTextureShader::initialize()
{
    return MS::kSuccess;
}

/* virtual */
MStatus	
hwManagedTextureShader::glBind(const MDagPath& path)
{
	// ONLY push and pop required attributes performance reasons...
	//
	glPushAttrib(GL_LIGHTING_BIT);

	lightingOn = glIsEnabled(GL_LIGHTING);

	if (lightingOn)
	{
		glEnable(GL_COLOR_MATERIAL);
		glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	}

	// Base colour is always white
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	// Bind texture 
#if MAYA_API_VERSION >= 800
	MObject object = path.node();
	MFnMesh mesh(object);
	MString uvSetName("map1");
	MObjectArray textures;

	boundTexture = false;
	MStatus status = mesh.getAssociatedUVSetTextures(uvSetName, textures);
	if (status == MS::kSuccess && textures.length())
	{
		MImageFileInfo::MHwTextureType hwType;
		if (MS::kSuccess == MHwTextureManager::glBind( textures[0], hwType ))
		{
			boundTexture = true;
		}
	}

	if( !boundTexture)
	{
		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
#else
	// To get this code branch to compile, replace <change file name here>
	// with an appropriate file name
	static GLuint id = 0;
	if (id == 0)
	{
		MImage fileImage;
		MStatus status = fileImage.readFromFile("<change file name here>");
		glGenTextures(1, &id);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, id);
		GLuint width, height;
        fileImage.getSize( width, height);
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, fileImage.pixels());
	}
	else
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, id);
	}
	boundTexture = true;

#endif
	if( boundTexture)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	glEnableClientState(GL_VERTEX_ARRAY);

	return MS::kSuccess;
}

/* virtual */
MStatus	
hwManagedTextureShader::glUnbind(const MDagPath& shapePath)
{
	// Cleanup GL state, without using pushing and popping attributes
	//
	glDisableClientState(GL_VERTEX_ARRAY);
	if (lightingOn)
	{
		glDisable(GL_COLOR_MATERIAL);
		glDisableClientState(GL_NORMAL_ARRAY);
	}
	if (boundTexture)
	{
		glDisableClientState( GL_TEXTURE_COORD_ARRAY );
		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// ONLY push and pop required attributes performance reasons...
	//
	glPopAttrib();
    return MS::kSuccess;
}

bool
hwManagedTextureShader::supportsBatching() const
{
	return true;
}

/* virtual */
MStatus	hwManagedTextureShader::glGeometry( const MDagPath& path,
                                int prim,
								unsigned int writable,
								int indexCount,
								const unsigned int * indexArray,
								int vertexCount,
								const int * vertexIDs,
								const float * vertexArray,
								int normalCount,
								const float ** normalArrays,
								int colorCount,
								const float ** colorArrays,
								int texCoordCount,
								const float ** texCoordArrays)
{
	glVertexPointer ( 3, GL_FLOAT, 0, &vertexArray[0] );

	if (boundTexture && texCoordCount && texCoordArrays[0] != NULL)
	{
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glTexCoordPointer( 2, GL_FLOAT, 0, &texCoordArrays[0][0] );
	}
	else
	{
		glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	}

	if (lightingOn && normalCount && normalArrays[0][0])
	{
		// Don't route normals if we don't need them
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer ( GL_FLOAT, 0, &normalArrays[0][0] );
	}
	else
	{
		glDisableClientState( GL_NORMAL_ARRAY );
	}

	glDrawElements ( prim, indexCount, GL_UNSIGNED_INT, indexArray );

	return MS::kSuccess;
}

// virtual
int	hwManagedTextureShader::texCoordsPerVertex()
{
	return 1;
}

// virtual
int	hwManagedTextureShader::normalsPerVertex()
{
	return 1;
}



//////////////////////////////////////////////////////////////
MStatus initializePlugin( MObject obj )
{ 
	MStatus   status;

	const MString UserClassify( "shader/surface/utility/" );

	MFnPlugin plugin( obj, PLUGIN_COMPANY, "8.0", "Any");

	status = plugin.registerNode("hwManagedTextureShader",
								 hwManagedTextureShader::id, 
								 hwManagedTextureShader::creator,
								 hwManagedTextureShader::initialize,
								 MPxNode::kHwShaderNode, &UserClassify );
	CHECK_MSTATUS(status);

	return status;
}

MStatus uninitializePlugin( MObject obj )
{
	MStatus   status;
	MFnPlugin plugin( obj );

	status = plugin.deregisterNode( hwManagedTextureShader::id );
	CHECK_MSTATUS(status);

	return status;
}


