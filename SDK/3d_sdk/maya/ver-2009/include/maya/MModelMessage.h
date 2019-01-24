#ifndef _MModelMessage
#define _MModelMessage
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
// CLASS:    MModelMessage
//
// ****************************************************************************

#if defined __cplusplus

// ****************************************************************************
// INCLUDED HEADER FILES


#include <maya/MMessage.h>

// ****************************************************************************
// DECLARATIONS

class MObject;

// ****************************************************************************
// CLASS DECLARATION (MModelMessage)

//! \ingroup OpenMaya
//! \brief Scene messages. 
/*!
	This class is used to register callbacks for model related messages.

    The addCallback method registers a function that will be executed
    whenever the specified message occurs. An id is returned and is used
	to remove the callback.

    To remove a callback use MMessage::removeCallback.
	All callbacks that are registered by a plug-in must be removed by that
	plug-in when it is unloaded. Failure to do so will result in a fatal error.
*/
class OPENMAYA_EXPORT MModelMessage : public MMessage
{
public:
	//! Message types supported by the addCallback method.
	enum Message {
		kActiveListModified	//!< active selection changes
   };

public:
	static MCallbackId	addCallback( Message, MMessage::MBasicFunction,
									 void * clientData = NULL,
									 MStatus * ReturnStatus = NULL );


	//!	Before duplicate callback
	static MCallbackId	addBeforeDuplicateCallback(
								MMessage::MBasicFunction,
								void * clientData = NULL,
								MStatus * ReturnStatus = NULL );

	//!	After duplicate callback
	static MCallbackId	addAfterDuplicateCallback(
								MMessage::MBasicFunction,
								void * clientData = NULL,
								MStatus * ReturnStatus = NULL );

	//! Node added to model message
	static MCallbackId	addNodeAddedToModelCallback(
								const MObject &dagNode,
								MMessage::MNodeFunction,
								void * clientData = NULL,
								MStatus * ReturnStatus = NULL );

	//! Node removed from model message
	static MCallbackId	addNodeRemovedFromModelCallback(
								const MObject &dagNode,
								MMessage::MNodeFunction,
								void * clientData = NULL,
								MStatus * ReturnStatus = NULL );

private:
	static const char* className();
};

#endif /* __cplusplus */
#endif /* _MModelMessage */
