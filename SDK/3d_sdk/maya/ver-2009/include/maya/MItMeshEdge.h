#ifndef _MItMeshEdge
#define _MItMeshEdge
//-
// ==========================================================================
// Copyright (C) 1995 - 2006 Autodesk, Inc., and/or its licensors.  All
// rights reserved.
//
// The coded instructions, statements, computer programs, and/or related
// material (collectively the "Data") in these files contain unpublished
// information proprietary to Autodesk, Inc. ("Autodesk") and/or its
// licensors,  which is protected by U.S. and Canadian federal copyright law
// and by international treaties.
//
// The Data may not be disclosed or distributed to third parties or be
// copied or duplicated, in whole or in part, without the prior written
// consent of Autodesk.
//
// The copyright notices in the Software and this entire statement,
// including the above license grant, this restriction and the following
// disclaimer, must be included in all copies of the Software, in whole
// or in part, and all derivative works of the Software, unless such copies
// or derivative works are solely in the form of machine-executable object
// code generated by a source language processor.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
// AUTODESK DOES NOT MAKE AND HEREBY DISCLAIMS ANY EXPRESS OR IMPLIED
// WARRANTIES INCLUDING, BUT NOT LIMITED TO, THE WARRANTIES OF
// NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE,
// OR ARISING FROM A COURSE OF DEALING, USAGE, OR TRADE PRACTICE. IN NO
// EVENT WILL AUTODESK AND/OR ITS LICENSORS BE LIABLE FOR ANY LOST
// REVENUES, DATA, OR PROFITS, OR SPECIAL, DIRECT, INDIRECT, OR
// CONSEQUENTIAL DAMAGES, EVEN IF AUTODESK AND/OR ITS LICENSORS HAS
// BEEN ADVISED OF THE POSSIBILITY OR PROBABILITY OF SUCH DAMAGES.
// ==========================================================================
//+
//
// CLASS:    MItMeshEdge
//
// ****************************************************************************

#if defined __cplusplus

// ****************************************************************************
// INCLUDED HEADER FILES


#include <maya/MFnDagNode.h>
#include <maya/MObject.h>
#include <maya/MVector.h>
#include <maya/MPoint.h>

// ****************************************************************************
// DECLARATIONS

class MPointArray;
class MDoubleArray;
class MIntArray;

// ****************************************************************************
// CLASS DECLARATION (MItMeshEdge)

//! \ingroup OpenMaya
//! \brief Polygon edge iterator. 
/*!
	This class is the edge iterator for polygonal surfaces.

	The iterator functions in two modes depending on whether a component
	is specified. When a component is not given or is NULL the iteration
	will be over all edges for the polygon.  When a component is given
	this iterator will iterate over the edges specified in the component.
	When iterating over components a DAG path to the polygon must also be
	supplied.
*/
class OPENMAYA_EXPORT MItMeshEdge
{
public:
    MItMeshEdge( MObject & polyObject, MStatus * ReturnStatus = NULL );
	MItMeshEdge( MObject & polyObject, MObject & component, MStatus * ReturnStatus = NULL );
    MItMeshEdge( const MDagPath &polyObject, MObject & component = MObject::kNullObj, MStatus * ReturnStatus = NULL );
	virtual ~MItMeshEdge();
    bool        isDone( MStatus * ReturnStatus = NULL );
    MStatus     next();
    MStatus     reset();
    MStatus     reset( MObject & polyObject, MObject & component = MObject::kNullObj );
    MStatus     reset( const MDagPath &polyObject, MObject & component = MObject::kNullObj );
    int        count( MStatus * ReturnStatus = NULL );
    MPoint      center( MSpace::Space space = MSpace::kObject,
						MStatus * ReturnStatus = NULL );
    MPoint      point( int index, MSpace::Space space = MSpace::kObject,
						MStatus * ReturnStatus = NULL );
    MStatus     setPoint( const MPoint & point, unsigned int index,
						MSpace::Space space = MSpace::kObject );
	bool		isSmooth( MStatus * ReturnStatus = NULL ) const;
	MStatus		setSmoothing( bool smooth = true );
	// Obsolete
	MStatus		cleanupSmoothing();
    int         index( int index, MStatus * ReturnStatus = NULL );
    int         index( MStatus * ReturnStatus = NULL ) const;
	// Obsolete
	MObject		edge( MStatus * ReturnStatus = NULL );
	MObject		currentItem( MStatus * ReturnStatus = NULL );
    MStatus     updateSurface();
    MStatus     geomChanged();
	MStatus		setIndex(int index, int &prevIndex );
	int			getConnectedFaces(MIntArray & faceList,
								  MStatus * ReturnStatus = NULL ) const;
	int			getConnectedEdges(MIntArray & edgeList,
								  MStatus * ReturnStatus = NULL ) const;
	MStatus		numConnectedFaces(int &faceCount ) const;
	MStatus		numConnectedEdges(int &edgeCount ) const;
	bool		connectedToFace( int index, MStatus * ReturnStatus = NULL);
	bool		connectedToEdge( int index, MStatus * ReturnStatus = NULL);
	bool		onBoundary(MStatus * ReturnStatus = NULL );
	MStatus		getLength(double &length,
						MSpace::Space space = MSpace::kObject );

protected:
// No protected members

private:
    static const char*  className();
    void     *       f_it;
    MPtrBase *       f_shape;
    void	 *       f_path;
    void	 *       f_geom;
	void     *       fElements;
	int              fCurrentElement;
	int              fMaxElements;
	int				 fCurrentIndex;
	void	*		 f_edge;
	void	*		 f_ref;
	bool			 fDirectIndex;
	bool			 fCustomEdgeComponent;
};

#endif /* __cplusplus */
#endif /* _MItMeshEdge */
