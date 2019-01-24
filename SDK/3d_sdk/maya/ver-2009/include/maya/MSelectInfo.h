#ifndef _MSelectInfo
#define _MSelectInfo
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
// CLASS:    MSelectInfo
//
// ****************************************************************************

#if defined __cplusplus

// ****************************************************************************
// INCLUDED HEADER FILES


#include <maya/MDrawInfo.h>

// ****************************************************************************
// DECLARATIONS

class MSelectionMask;
class MPoint;
class MPointArray;
class MVector;
class MSelectionList;
class MMatrix;

// ****************************************************************************
// CLASS DECLARATION (MSelectInfo)

//! \ingroup OpenMayaUI
//! \brief Selection state information used in MPxSurfaceShapeUI::select
/*!
MSelectInfo is used with user defined shape selection and is passed
as an argument to the MPxSurfaceShapeUI::select method.
This class encapsulates all the selection state information for
selected objects.
*/
class OPENMAYAUI_EXPORT MSelectInfo : public MDrawInfo
{
public:
	MSelectInfo();
	MSelectInfo( const MSelectInfo& in );
	~MSelectInfo();

public:

	// Public accessor methods

    M3dView			view();

	bool			singleSelection() const;
	bool			selectClosest() const;
    bool			selectable( MSelectionMask & mask ) const;
    bool			selectableComponent( bool displayed,
										 MSelectionMask & mask ) const;

    bool			isRay() const;
	MMatrix			getAlignmentMatrix() const;
    void			getLocalRay( MPoint&, MVector & ) const;

    bool			selectForHilite( const MSelectionMask & ) const;

    bool			selectOnHilitedOnly() const;

	int				highestPriority() const;
	void			setHighestPriority( int );

    void			addSelection( const MSelectionList &item,
								  const MPoint &point,
								  MSelectionList &list,
								  MPointArray &points,
								  const MSelectionMask & mask,
								  bool isComponent );

    MDagPath		selectPath() const;


protected:
// No protected members

private:
	const char*	 className() const;
    MSelectInfo( void* in );
};

#endif /* __cplusplus */
#endif /* _MSelectInfo */
