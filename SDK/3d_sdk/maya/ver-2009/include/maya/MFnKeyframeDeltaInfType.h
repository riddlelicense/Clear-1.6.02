#ifndef _MFnKeyframeDeltaInfType
#define _MFnKeyframeDeltaInfType
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
//
// ****************************************************************************
//
// CLASS:    MFnKeyframeDeltaInfType
//
// ****************************************************************************

#if defined __cplusplus

// ****************************************************************************
// INCLUDED HEADER FILES


#include <maya/MFnKeyframeDelta.h>
#include <maya/MFnAnimCurve.h>

// ****************************************************************************
// CLASS DECLARATION (MFnKeyframeDeltaInfType)

//! \ingroup OpenMayaAnim MFn
//! \brief Function set for changes in pre or post infinity type. 
/*!
This delta object represents changes in the pre-infinity or
post-infinity type of the animation curve. Pre-infinity and
post-infinity types are specific to animation curves and do not have a
corresponding key associated with them.  It is, therefore, invalid to
have a keyframe associated with a MFnKeyframeDeltaInfType. To
determine if this class represents a change to pre or post infinity
use the method isPreInfinity on this class.
*/
class OPENMAYAANIM_EXPORT MFnKeyframeDeltaInfType : public MFnKeyframeDelta
{
	declareMFn(MFnKeyframeDeltaInfType, MFnKeyframeDelta);

public:

	MFnAnimCurve::InfinityType	previousInfinityType( MStatus *ReturnStatus = NULL ) const;
	MFnAnimCurve::InfinityType  currentInfinityType( MStatus *ReturnStatus = NULL ) const;

	bool    isPreInfinity( MStatus *ReturnStatus = NULL ) const;

BEGIN_NO_SCRIPT_SUPPORT:

 	declareMFnConstConstructor( MFnKeyframeDeltaInfType, MFnKeyframeDelta );

END_NO_SCRIPT_SUPPORT:

protected:
	// No protected.

private:
	// No private.
};

#endif /* __cplusplus */
#endif /* _MFnKeyframeDeltaInfType */
