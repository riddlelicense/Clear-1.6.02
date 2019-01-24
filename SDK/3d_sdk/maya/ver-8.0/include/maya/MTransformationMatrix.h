#ifndef _MTransformationMatrix
#define _MTransformationMatrix
//
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
// CLASS:    MTransformationMatrix
//
// *****************************************************************************
//
// CLASS DESCRIPTION (MTransformationMatrix)
//
// An MTransformationMatrix allows the manipulation of the individual
// transformation components (eg scale, rotation, shear, etc) of a four by
// four transformation matrix.
//
// *****************************************************************************

#if defined __cplusplus

// *****************************************************************************

// INCLUDED HEADER FILES


#include <maya/MStatus.h>
#include <maya/MTypes.h>

// *****************************************************************************

// DECLARATIONS

class MMatrix;
class MVector;
class MPoint;
class MQuaternion;
class MEulerRotation;

#define MTransformationMatrix_kTol 1.0e-10

// *****************************************************************************

// CLASS DECLARATION (MTransformationMatrix)

/// Transformation matrix
/**
	Class for constructing transformation matrices
*/
#ifdef _WIN32
#pragma warning(disable: 4522)
#endif // _WIN32

class OPENMAYA_EXPORT MTransformationMatrix  
{
public:
    ///
	enum RotationOrder {
        ///
		kInvalid,
        ///
		kXYZ,
        ///
		kYZX,
        ///
		kZXY,
        ///
		kXZY,
        ///
		kYXZ,
        ///
		kZYX,
        ///
		kLast
	};
    ///
	~MTransformationMatrix();
    ///
	MTransformationMatrix();
    ///
	MTransformationMatrix( const MTransformationMatrix& src );
    ///
	MTransformationMatrix( const MMatrix& src );
    ///
	MTransformationMatrix&   	operator= ( const MTransformationMatrix &src );
    ///
	MTransformationMatrix&   	operator= ( const MMatrix &src );
    ///
	MMatrix		asMatrix()					const;
    ///
 	MMatrix		asMatrix(double percent)	const;
    ///
 	MMatrix		asMatrixInverse()			const;
    ///
	MMatrix		asScaleMatrix()				const;
    ///
 	MMatrix		asRotateMatrix()			const;
    ///
 	MStatus		getScale( double[3], MSpace::Space ) const;
    ///
 	MStatus		setScale( const double[3], MSpace::Space );
    ///
 	MStatus		addScale( const double[3], MSpace::Space );
	///
	MQuaternion	rotation() const;
	///
	MTransformationMatrix &rotateTo( const MQuaternion &q );
	///
	MTransformationMatrix &rotateBy( const MQuaternion &q, 
									 MSpace::Space space,
									 MStatus *ReturnStatus = NULL);
	///
	MEulerRotation	eulerRotation() const;
	///
	MTransformationMatrix &rotateTo( const MEulerRotation &e );
	///
	MTransformationMatrix &rotateBy( const MEulerRotation &e, 
									 MSpace::Space space,
									 MStatus *ReturnStatus = NULL);
    ///
 	MStatus		getRotationQuaternion( double &x, double &y, double &z,
									   double &w ) const;
    ///
 	MStatus		setRotationQuaternion( double x, double y, double z,
									   double w );
    ///
 	MStatus		addRotationQuaternion( double x, double y, double z,
									   double w, MSpace::Space );
    ///
 	MStatus		getRotation( double[3],
							 RotationOrder& order ) const;
    ///
 	MStatus		setRotation( const double rotation[3],
							 RotationOrder order );
    ///
	MStatus		addRotation( const double rotation[3],
							 RotationOrder order, MSpace::Space );
    ///
	RotationOrder	rotationOrder( MStatus* ReturnStatus = NULL ) const;
    ///
	MStatus	    reorderRotation( RotationOrder& order );
	///
	MStatus     setToRotationAxis( const MVector &axis, double rotation );
	///
	MQuaternion rotationOrientation() const;
	///
	MTransformationMatrix &setRotationOrientation( const MQuaternion &q );
    ///
	MVector		getTranslation( MSpace::Space, MStatus* ReturnStatus=NULL ) const;
    ///
 	MStatus		setTranslation( const MVector& vector, MSpace::Space );
    ///
	MStatus		addTranslation( const MVector& vector, MSpace::Space );
    ///
	MStatus		setShear( const double[3], MSpace::Space );
    ///
	MStatus		getShear( double[3], MSpace::Space ) const;
    ///
	MStatus		addShear( const double[3], MSpace::Space );
    ///
	MPoint		scalePivot( MSpace::Space, MStatus* ReturnStatus = NULL ) const;
    ///
	MStatus		setScalePivot( const MPoint&, MSpace::Space, bool balance );
    ///
	MVector		scalePivotTranslation(MSpace::Space,
									  MStatus* ReturnStatus=NULL )const;
    ///
	MStatus		setScalePivotTranslation( const MVector& vector,
										  MSpace::Space );
    ///
	MPoint		rotatePivot( MSpace::Space, MStatus* ReturnStatus=NULL ) const;
    ///
	MStatus		setRotatePivot( const MPoint& point,
								MSpace::Space, bool balance );
    ///
	MVector		rotatePivotTranslation( MSpace::Space space,
					MStatus* ReturnStatus=NULL ) const;
    ///
	MStatus		setRotatePivotTranslation( const MVector& vector,
										   MSpace::Space space );
    ///
 	bool		isEquivalent ( const MTransformationMatrix &other,
					double tolerance = MTransformationMatrix_kTol ) const;
    ///
 	bool		operator == (const MTransformationMatrix & other ) const;
    ///
	bool		operator != ( const MTransformationMatrix & other ) const;

    /// OBSOLETE
 	MStatus		getRotation( double[3],
							 RotationOrder& order, MSpace::Space ) const;
    /// OBSOLETE
 	MStatus		setRotation( const double rotation[3],
							 RotationOrder order, MSpace::Space );
    /// OBSOLETE
 	MStatus		getRotationQuaternion( double &x, double &y, double &z,
									   double &w, MSpace::Space ) const;
    /// OBSOLETE
 	MStatus		setRotationQuaternion( double x, double y, double z,
									   double w, MSpace::Space );
    /// OBSOLETE
	MVector		translation( MSpace::Space, MStatus* ReturnStatus=NULL ) const;

	///
	static const MTransformationMatrix identity;


protected:
// No protected members

private:
	const void*		data;
	const char*	className() const;
	MTransformationMatrix( const void* );
};

#ifdef _WIN32
#pragma warning(default: 4522)
#endif // _WIN32

// *****************************************************************************
#endif /* __cplusplus */
#endif /* _MTransformationMatrix */
