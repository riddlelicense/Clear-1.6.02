/*
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
*/

// 
// RTG Translator Maya specific source
//

#include <stdarg.h>
#include <stdlib.h>

#if defined (LINUX)
#include <sys/param.h>
#include <sys/stat.h>
#endif

// #include <maya/NTDLL.h>

#include <maya/MStatus.h>
#include <maya/MObject.h>
#include <maya/MFnPlugin.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MPxFileTranslator.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MItSelectionList.h>
#include <maya/MSelectionList.h>

#include <maya/MArgList.h>
#include <maya/MAnimControl.h>

#ifdef AW_NEW_IOSTREAMS
#include <iostream>
#include <fstream>
#else
#include <iostream.h>
#include <fstream.h>
#endif
#include <string.h>


#include <MDt.h>
#include <MDtExt.h>

#ifdef WIN32
#define MAXPATHLEN 512
#endif

enum { VRHRC_WORLD=0,VRHRC_FLAT,VRHRC_FULL };
enum { VRSEL_ALL=0,VRSEL_PICKED,VRSEL_ACTIVE };

//	Allow us to keep track of the version number

extern char *rtgVersion;

//
//  variables for keeping track of the UI interface options
//

extern int		rtg_v18_compatible;

extern int		rtg_output_file_format;
extern int		rtg_output_image_format;

extern int     rtg_output_vert_norms  ;
extern int     rtg_output_vert_colors ;
extern int     rtg_output_tex_coords ;
extern int     rtg_output_poly_norms ;
extern int     rtg_output_hierarchy  ;
extern int     rtg_show_index_counters ;
extern int     rtg_output_pivots     ;
extern int     rtg_output_transforms ;
extern int     rtg_output_materials  ;
extern int     rtg_output_animation  ;
extern int     rtg_output_local      ;
extern int     rtg_output_all_nodes  ;
extern int     rtg_output_degrees    ;
extern int     rtg_output_decomp     ;


MString scriptToRun;
int     scriptAppend = 0;

//
//	external functions
//

extern	int rtgExport();

/* ====================================== */
/* Get path name from a full path	 */
/* ====================================== */
char *
MDt_GetPathName (char *LFullStr, char *LRet, int LMaxLen)
{
	register int	LC;
	register int    LLen,
	                LLastSlashPos;

	LLastSlashPos = -1;
	for (LC = 0; LFullStr[LC] != '\0'; LC++)
	{
		if (LFullStr[LC] == '/')
			LLastSlashPos = LC;
	}

	LLen = LLastSlashPos + 1;

	if (LRet == NULL)
		return (NULL);

	LC = 0;
	if ((LRet != NULL) && ((LLen + 1) <= LMaxLen))
	{
		for (; LC < LLen; LC++)
			LRet[LC] = LFullStr[LC];
	}
	LRet[LC] = '\0';

	return (LRet);
}



class rtgTranslator : public MPxFileTranslator {

public:

	rtgTranslator () { };
	virtual ~rtgTranslator () { };
	static void    *creator ();

	MStatus         reader ( const MFileObject& file,
			                 const MString& optionsString,
			                 MPxFileTranslator::FileAccessMode mode);
	MStatus         writer ( const MFileObject& file,
		                     const MString& optionsString,
		                     MPxFileTranslator::FileAccessMode mode);
	bool            haveReadMethod () const;
	bool            haveWriteMethod () const;
	MString         defaultExtension () const;
	bool            canBeOpened () const;
	MFileKind       identifyFile ( const MFileObject& fileName,
				                   const char *buffer,
				                   short size) const;

private:
	static MString  magic;
};

void* rtgTranslator::creator()
{
	return new rtgTranslator();
}



// Initialize our magic "number"
MString rtgTranslator::magic ("HEADER_TITLE");

// An rtg file is an ascii file where the 1st line contains 
// the string HEADER_TITLE 
//
//
// The reader is not implemented yet.
//
MStatus rtgTranslator::reader (const MFileObject & file,
								const MString & options,
								MPxFileTranslator::FileAccessMode mode)
{
	MStatus rval (MS::kSuccess);

	return rval;
}

// Write method of the rtg translator / file exporter
//

MStatus rtgTranslator::writer ( const MFileObject & fileObject,
								  const MString & options,
								  MPxFileTranslator::FileAccessMode mode)
{
	char            LTmpStr[MAXPATHLEN];
	unsigned int	i;
	int             LN;

	const MString   fname = fileObject.fullName ();
	MString         extension;
	MString         baseFileName;

    int             TimeSlider = 0;
    int             AnimEnabled = 0;


	// Lets strip off the known extension of .rtg if it is there.
	
	extension.set (".rtg");
	int  extLocation = fileObject.name ().rindex ('.');

	if (extLocation > 0 && fileObject.name ().substring (extLocation,
			     fileObject.name ().length () - 1) == extension)
	{
		baseFileName = fileObject.name ().substring (0, extLocation - 1);
	} else
	{
		baseFileName = fileObject.name ();
		extension.clear ();
	}


    DtExt_SceneInit( (char *)baseFileName.asChar() );
	
	// Lets now do all of the option processing
	
	if (options.length () > 0)
	{
		//Start parsing.

		MStringArray optionList;
		MStringArray    theOption;

		options.split (';', optionList);

		//break out all the options.

		for ( i = 0; i < optionList.length (); ++i)
		{
			theOption.clear ();
			optionList[i].split ('=', theOption);
			if (theOption.length () > 1)
			{
				if (theOption[0] == MString ("v18compatible"))
				{
					rtg_v18_compatible = (int) (theOption[1].asInt() );
					
				} else if (theOption[0] == MString ("timeslider"))
				{
					TimeSlider = (int) (theOption[1].asInt ());
                } else if (theOption[0] == MString ("animEnabled"))
                {
                    AnimEnabled = (int) (theOption[1].asInt ());

				} else if (theOption[0] == MString ("animStart"))
				{
                    DtFrameSetStart( (int) (theOption[1].asInt ()) );

				} else if (theOption[0] == MString ("animEnd"))
				{
                    DtFrameSetEnd( (int) (theOption[1].asInt ()) );

				} else if (theOption[0] == MString ("animStep"))
				{
                    DtFrameSetBy( (int) (theOption[1].asInt ()) );

				} else if (theOption[0] == MString ("hrcType"))
				{
                    switch ( theOption[1].asInt () - 1)
                    {
                        case VRHRC_FLAT:
                            DtExt_setOutputTransforms (kTRANSFORMMINIMAL);
                            DtExt_setParents (0);
                            break;
                            
                        case VRHRC_WORLD:
                            DtExt_setOutputTransforms (kTRANSFORMNONE);
                            DtExt_setParents (0);
                            break;
                            
                        case VRHRC_FULL:
                        default:
                            DtExt_setOutputTransforms (kTRANSFORMALL);
                            DtExt_setParents (1);
                            break;
                    }       

                } else if (theOption[0] == MString ("joints"))
                { 
                    // Allow user to specify if the hierarchy should include
					// NULL geometry nodes - usually joints 
                    
                    DtExt_setJointHierarchy( theOption[1].asInt() );
 
				} else if (theOption[0] == MString ("exportSel"))
				{
                    switch ( theOption[1].asInt () - 1)
                    {
                        case VRSEL_ALL:
                            DtExt_setWalkMode (ALL_Nodes);
                            break;
                        case VRSEL_ACTIVE:
                            DtExt_setWalkMode (ACTIVE_Nodes);
                            break;
                        case VRSEL_PICKED:
                            DtExt_setWalkMode (PICKED_Nodes);
                            break;
                    }      
				} else if (theOption[0] == MString ("texsample"))
				{
                    // Allow user to specify if the textures should be sampled 
                    // with the Texture Placement options
                    
                    DtExt_setSoftTextures ( theOption[1].asInt() );

				} else if (theOption[0] == MString ("texevaluate"))
				{
                    // Allow the user to specify if the tex should be eval with
                    // convertSolidTx command or read in if is a file texture.
                    
                    DtExt_setInlineTextures( theOption[1].asInt() );

				} else if (theOption[0] == MString ("texoriginal"))
				{
                    // Allow the user to specify if the tex should be eval at all.
                    
                    DtExt_setOriginalTexture( theOption[1].asInt() );
					
				} else if (theOption[0] == MString ("Xtexres"))
				{
                    // Set the X size of the texture swatches to use  
                    
                    DtExt_setXTextureRes ( theOption[1].asInt () );

				} else if (theOption[0] == MString ("Ytexres"))
				{
                    // Set the Y size of the texture swatches to use  
                    DtExt_setYTextureRes ( theOption[1].asInt () );

				} else if (theOption[0] == MString ("MaxXtexres"))
				{
                    // Set the Max X size of the texture swatches to use  
                    DtExt_setMaxXTextureRes( theOption[1].asInt () );
                
				} else if (theOption[0] == MString ("MaxYtexres"))
				{
                    // Set the Max Y size of the texture swatches to use  
                    DtExt_setMaxYTextureRes( theOption[1].asInt () );

				} else if (theOption[0] == MString ("precision"))
				{
					//VR_Precision = theOption[1].asInt ();
				} else if (theOption[0] == MString ("verbose"))
				{
                   // DtExt_setDebug ( theOption[1].asInt () );

                } else if (theOption[0] == MString ("debug"))
                { 
                    int levelG = DtExt_Debug();
                    if ( (int) (theOption[1].asInt () ) )
                        levelG |= DEBUG_GEOMAT;
                    else 
                        levelG &= ~DEBUG_GEOMAT;
                        
                    DtExt_setDebug( levelG );
                    
                } else if (theOption[0] == MString ("debugC"))
                { 
                    int levelC = DtExt_Debug();
                    if ( (int) (theOption[1].asInt () ) )
                        levelC |= DEBUG_CAMERA;
                    else 
                        levelC &= ~DEBUG_CAMERA;
                        
                    DtExt_setDebug( levelC );
                    
                } else if (theOption[0] == MString ("debugL"))
                { 
                    int levelL = DtExt_Debug();
                    if ( (int) (theOption[1].asInt () ) )
                        levelL |= DEBUG_LIGHT;
                    else 
                        levelL &= ~DEBUG_LIGHT;
                        
                    DtExt_setDebug( levelL );

				} else if (theOption[0] == MString ("reversed"))
				{
					DtExt_setWinding( theOption[1].asInt() );

				} else if (theOption[0] == MString ("tesselation"))
				{
					if ( theOption[1].asInt() == 2 )
					{
						DtExt_setTesselate( kTESSQUAD );
					} else {
						DtExt_setTesselate( kTESSTRI );
					}

				//
				// Now come the translator specific options
				//

                } else if (theOption[0] == MString ("imageformat"))
                {
					rtg_output_image_format = theOption[1].asInt();

                } else if (theOption[0] == MString ("fileformat"))
                {
                    rtg_output_file_format = theOption[1].asInt();

                } else if (theOption[0] == MString ("vnormals"))
                { 
                    rtg_output_vert_norms = theOption[1].asInt();

                } else if (theOption[0] == MString ("vcolors"))
                { 
                    rtg_output_vert_colors = theOption[1].asInt();

                } else if (theOption[0] == MString ("tcoords"))
                { 
                    rtg_output_tex_coords = theOption[1].asInt();

                } else if (theOption[0] == MString ("pnormals"))
                { 
                    rtg_output_poly_norms = theOption[1].asInt();

                } else if (theOption[0] == MString ("idxcnt"))
                { 
                    rtg_show_index_counters = theOption[1].asInt();

                } else if (theOption[0] == MString ("anglesdeg"))
                { 
                    rtg_output_degrees = theOption[1].asInt();

                } else if (theOption[0] == MString ("materials"))
                { 
                    rtg_output_materials = theOption[1].asInt();

                } else if (theOption[0] == MString ("multitexture"))
                {
                    DtExt_setMultiTexture( theOption[1].asInt() );

                } else if (theOption[0] == MString ("mdecomp"))
                { 
                    rtg_output_decomp = theOption[1].asInt();

                } else if (theOption[0] == MString ("pivoth"))
                { 
                    rtg_output_pivots = theOption[1].asInt();

                } else if (theOption[0] == MString ("transforms"))
                { 
                    rtg_output_transforms = theOption[1].asInt();

                } else if (theOption[0] == MString ("ltransforms"))
                { 
                    rtg_output_local = theOption[1].asInt();

                } else if (theOption[0] == MString ("animation"))
                { 
                    rtg_output_animation = theOption[1].asInt();

                } else if (theOption[0] == MString ("allnodes"))
                { 
                    rtg_output_all_nodes = theOption[1].asInt();

                } else if (theOption[0] == MString ("script"))
                {
                    scriptToRun = theOption[1];

                } else if (theOption[0] == MString ("scriptAppend"))
                {
                    scriptAppend = (int)(theOption[1].asInt() );
                }

			}
		}
	}

	// Lets see how we entered this plug-in, either with the export all
	// or export selection flag set.

    if ( mode == MPxFileTranslator::kExportActiveAccessMode )
	{ 
        DtExt_setWalkMode ( ACTIVE_Nodes );
	}

    // Lets check the TimeSlider control now:

    if ( TimeSlider )
    {
        MTime start( MAnimControl::minTime().value(), MTime::uiUnit() );
		DtFrameSetStart( (int) start.value() );

        MTime end( MAnimControl::maxTime().value(), MTime::uiUnit() );
		DtFrameSetEnd( (int) end.value() );
    }


    // Now see if the animation is really enabled.
    // Else we will set the end frame to the beginning frame automatically

    if ( !AnimEnabled )
    {
        DtFrameSetEnd( DtFrameGetStart() );
    }

	// Find out where the file is supposed to end up.
	
	MDt_GetPathName ((char *) (fname.asChar ()), LTmpStr, MAXPATHLEN);

	LN = (int)strlen (LTmpStr);
	if (LTmpStr[LN - 1] == '/')
		LTmpStr[LN - 1] = '\0';

	DtSetDirectory (LTmpStr);

    // Now lets setup some paths to do basic texture file searching
    // for those textures with relative paths
    
    MStringArray wSpacePaths;
    MStringArray rPaths;
    MString      usePath;
    MString      separator;
    
    MGlobal::executeCommand( "workspace -q -rd", wSpacePaths );
    MGlobal::executeCommand( "workspace -q -rtl", rPaths );
    
    if ( DtExt_getTextureSearchPath() )
        separator.set( "|" );
    else
        separator.set( "" );
    
    for (i = 0; i < wSpacePaths.length (); ++i)
    {   
        for ( unsigned int j = 0; j < rPaths.length(); j++ )
        {   
            usePath = usePath + separator + wSpacePaths[i] + MString( "/" ) + rPaths[j];    
            separator.set( "|" );
            
            if ( rPaths[j] == MString( "sourceImages" ) )
                usePath = usePath + separator + wSpacePaths[i] + MString( "/" )
                + MString( "sourceimages" );
        
        }
    }
    
    DtExt_addTextureSearchPath( (char *)usePath.asChar() );
    

    // Now we can setup the database from the wire file geometry.
    // This is where all the Maya data are retrieved, cached, and processed.
    //
    
    // Say that we want to have camera info
    
    DtExt_setOutputCameras( 1 );
    
    //Now we can setup the database from the wire file geometry
    
    DtExt_dbInit();
    
	DtFrameSet( DtFrameGetStart() );

    // Now do the export

    rtgExport();
    


    // Now lets see if the user wants something else to be done
    
    if ( 0 < scriptToRun.length() )
    {
        if ( scriptAppend )
        {
            scriptToRun += MString( " " ) + MString( LTmpStr );
        }   
        
        system( scriptToRun.asChar() );
    }   

    // Clean house.
    //
    
    DtExt_CleanUp();

	
	return MS::kSuccess;
}

bool rtgTranslator::haveReadMethod () const
{
	return false;
}

bool rtgTranslator::haveWriteMethod () const
{
	return true;
}

// Whenever Maya needs to know the preferred extension of this file format,
// it calls this method.  For example, if the user tries to save a file called
// "test" using the Save As dialog, Maya will call this method and actually
// save it as "test.wrl2".  Note that the period should * not * be included in
// the extension.

MString rtgTranslator::defaultExtension () const
{
	return "rtg";
}

// This method tells Maya whether the translator can open and import files
// (returns true) or only import  files (returns false)

bool rtgTranslator::canBeOpened () const
{
	return true;
}

MPxFileTranslator::MFileKind rtgTranslator::identifyFile (
	      const MFileObject & fileName,
	      const char *buffer,
	      short size) const
{
	//Check the buffer for the "rtg" magic number, the
	// string "HEADER_TITLE"

	MFileKind rval = kNotMyFileType;

	if ((unsigned(size) >= magic.length ()) &&
	    (0 == strncmp (buffer, magic.asChar (), magic.length ())))
	{
		rval = kIsMyFileType;
	}
	return rval;
}


MStatus 
initializePlugin (MObject obj)
{
	MStatus         status;
	char			version[256];

	strcpy( version, rtgVersion );
	strcat( version, "." );
	strcat( version, DtAPIVersion() );
	
	MFnPlugin       plugin (obj, "RTG Translator for Maya", version, "Any");

	//Register the translator with the system

	status = plugin.registerFileTranslator ("rtg",
							"rtgTranslator.rgb",
							rtgTranslator::creator,
							"rtgTranslatorOpts", "",
							true);
	if (!status)
	{
		status.perror ("registerFileTranslator");
		//return status;
	}

	return status;
}

MStatus 
uninitializePlugin (MObject obj)
{
	MStatus         status;
	MFnPlugin       plugin (obj);

	status = plugin.deregisterFileTranslator ("rtg");
	if (!status)
	{
		status.perror ("deregisterFileTranslator");
		//return status;
	}

	return status;
}
