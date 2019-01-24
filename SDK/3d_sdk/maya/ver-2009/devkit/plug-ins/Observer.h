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
#ifndef OBSERVER_H
#define OBSERVER_H

#include <map>
#include <vector>
#include <string>

#include <IAshliFX.h>

#include "glExtensions.h"

//
//
//////////////////////////////////////////////////////////////////////
class stateItem {
  public:
    virtual ~stateItem() {};
    virtual void apply() = 0;
};

//
//
//////////////////////////////////////////////////////////////////////
class blendStateItem : public stateItem {
  // enable, src_factor, dst_factor, blend_func, blend factor
  GLenum m_enable;
  GLenum m_srcFactor;
  GLenum m_dstFactor;
  GLenum m_blendOp;

  //anonymous struct + anonymous union
  // this might be a bit too tricky for gcc, but it makes the cleanest code
  union {
    GLbitfield m_options;
    struct {
      unsigned int m_useEnable : 1;    //call glEnable
      unsigned int m_useBlend : 1;     //call glBlend
      unsigned int m_getSrcFactor : 1; //call glGet for srcFactor
      unsigned int m_getDstFactor : 1; //call glGet for dstFactor
      unsigned int m_useBlendFunc : 1; //call glBlendFunction
      unsigned int m_useBlendOp : 1;  //call glBlendOp
    };
  };
  public:
    blendStateItem() : m_enable(GL_FALSE), m_srcFactor(GL_ONE), m_dstFactor(GL_ZERO), m_blendOp(GL_FUNC_ADD), m_options(0) {};
    virtual ~blendStateItem() {};
    virtual void apply();

    friend class stateObserver;
};

//
//
//////////////////////////////////////////////////////////////////////
class depthStateItem : public stateItem {
  // enable, depth func, depth mask
  GLenum m_enable;
  GLenum m_depthFunc;
  GLboolean m_depthMask;

  //anonymous struct + anonymous union
  // this might be a bit too tricky for gcc, but it makes the cleanest code
  union {
    GLbitfield m_options;
    struct {
      unsigned int m_useEnable : 1;    //call glEnable
      unsigned int m_useDepthFunc : 1; //call glDepthFunc
      unsigned int m_useDepthMask : 1; //call glDepthMask
    };
  };

  public:
    depthStateItem() : m_enable(0), m_depthFunc(GL_LEQUAL), m_depthMask(GL_TRUE), m_options(0) {};
    virtual ~depthStateItem() {};
    virtual void apply();

    friend class stateObserver;
};

//
//
//////////////////////////////////////////////////////////////////////
class stencilStateItem : public stateItem {
  // enable, read mask, write mask, depth fail, stencil fail, stecil pass, ref
  GLenum m_enable;
  GLenum m_func;
  GLuint m_rmask;
  GLuint m_ref;
  GLenum m_depthPassOp;
  GLenum m_depthFailOp;
  GLenum m_stencilFailOp;
  GLuint m_mask;

  //anonymous struct + anonymous union
  // this might be a bit too tricky for gcc, but it makes the cleanest code
  union {
    GLbitfield m_options;
    struct {
      unsigned int m_useEnable : 1;    //call glEnable
      unsigned int m_useStencilFunc : 1;     //call glBlend
      unsigned int m_getStencilFunc : 1; //call glGet for srcFactor
      unsigned int m_getStencilRMask : 1; //call glGet for dstFactor
      unsigned int m_getStencilRef : 1; //call glBlendFunction
      unsigned int m_useStencilOp : 1;
      unsigned int m_getDepthPass : 1;
      unsigned int m_getDepthFail : 1;
      unsigned int m_getStencilFail : 1;
      unsigned int m_useStencilMask : 1;
    };
  };
  public:
    stencilStateItem() : m_enable(0), m_func(GL_EQUAL), m_rmask(0xff), m_ref(0), m_depthPassOp(GL_KEEP), m_depthFailOp(GL_KEEP),
      m_stencilFailOp(GL_KEEP), m_mask(0xff), m_options(0) {};
    virtual ~stencilStateItem() {};
    virtual void apply();

    friend class stateObserver;
};

//
//
//////////////////////////////////////////////////////////////////////
class primitiveStateItem : public stateItem {
  // polygon mode, cull, polygon offset
  GLenum m_polygonMode;
  GLenum m_enableCull;
  GLenum m_cullFace;
  GLenum m_enablePolygonOffset;
  GLfloat m_factor;
  GLfloat m_units;

  //anonymous struct + anonymous union
  // this might be a bit too tricky for gcc, but it makes the cleanest code
  union {
    GLbitfield m_options;
    struct {
      unsigned int m_usePolygonMode : 1;     //call glBlend
      unsigned int m_useEnableCull : 1; //call glGet for srcFactor
      unsigned int m_useCullFace : 1; //call glGet for dstFactor
      unsigned int m_useEnablePolygonOffset : 1; //call glBlendFunction
      unsigned int m_usePolygonOffset : 1; //call glBlendFunction
      unsigned int m_getFactor : 1; //call glBlendFunction
      unsigned int m_getUnits : 1; //call glBlendFunction
    };
  };
  public:
    primitiveStateItem() : m_polygonMode(GL_FILL), m_enableCull(GL_FALSE), m_cullFace(GL_BACK), m_enablePolygonOffset(GL_FALSE),
      m_factor(0.0f), m_units(0.0f), m_options(0) {};
    virtual ~primitiveStateItem() {};
    virtual void apply();

    friend class stateObserver;
};

//
//
//////////////////////////////////////////////////////////////////////
class alphaStateItem : public stateItem {
  // alpha_test_enable, alpha_func, ref
  GLenum m_enable;
  GLenum m_alphaFunc;
  GLfloat m_ref;

  //anonymous struct + anonymous union
  // this might be a bit too tricky for gcc, but it makes the cleanest code
  union {
    GLbitfield m_options;
    struct {
      unsigned int m_useEnable : 1;    //call glEnable
      unsigned int m_useAlphaFunc : 1;     //call glBlend
      unsigned int m_getAlphaFunc : 1; //call glGet for srcFactor
      unsigned int m_getRef : 1; //call glGet for dstFactor
    };
  };

  public:
    alphaStateItem() : m_enable(GL_FALSE), m_alphaFunc(GL_ALWAYS), m_ref(0.0f), m_options(0) {};
    virtual ~alphaStateItem() {};
    virtual void apply();

    friend class stateObserver;
};

//
//
//////////////////////////////////////////////////////////////////////
class colorStateItem : public stateItem {
  // dither, color mask
  GLenum m_dither;
  GLboolean m_mask[4];

  //anonymous struct + anonymous union
  // this might be a bit too tricky for gcc, but it makes the cleanest code
  union {
    GLbitfield m_options;
    struct {
      unsigned int m_useDither : 1;    //call glEnable
      unsigned int m_useMask : 1;     //call glBlend
    };
  };

  public:
    colorStateItem() : m_dither(GL_TRUE), m_options(0) {};
    virtual ~colorStateItem() {};
    virtual void apply();

    friend class stateObserver;
};

//
//
//////////////////////////////////////////////////////////////////////
class fogStateItem : public stateItem {
  // enable, start, end, density, color
  GLenum m_enable;
  GLenum m_mode;
  GLfloat m_start;
  GLfloat m_end;
  GLfloat m_density;
  GLfloat m_color[3];

  //anonymous struct + anonymous union
  // this might be a bit too tricky for gcc, but it makes the cleanest code
  union {
    GLbitfield m_options;
    struct {
      unsigned int m_useEnable : 1;    //call glEnable
      unsigned int m_useMode : 1;    //set Fog Mode
      unsigned int m_useFogStart : 1;     //call glBlend
      unsigned int m_useFogEnd : 1; //call glGet for srcFactor
      unsigned int m_useFogDensity : 1; //call glGet for dstFactor
      unsigned int m_useFogColor : 1; //call glBlendFunction
    };
  };

  public:
    fogStateItem() : m_enable(GL_FALSE), m_mode(GL_LINEAR), m_start(0.0f), m_end(1.0f), m_density(1.0f), m_options(0) {};
    virtual ~fogStateItem() {};
    virtual void apply();

    friend class stateObserver;
};

//
//
//////////////////////////////////////////////////////////////////////
class pointStateItem : public stateItem {
  GLfloat m_pointSize;
  GLfloat m_pointSizeMin;
  GLfloat m_pointSizeMax;
  GLfloat m_pointAtten[3];
  GLenum m_pointSprite;

  //anonymous struct + anonymous union
  // this might be a bit too tricky for gcc, but it makes the cleanest code
  union {
    GLbitfield m_options;
    struct {
      unsigned int m_usePointSize : 1;    //call glPointSzie
      unsigned int m_usePointSizeMin : 1;     //set Pt size Min
      unsigned int m_usePointSizeMax : 1; //set Pt size Max
      unsigned int m_usePointAtten : 1;  //set Pt Atten
      unsigned int m_getAttenA : 1;
      unsigned int m_getAttenB : 1;
      unsigned int m_getAttenC : 1;
      unsigned int m_usePointSprite : 1; //call glGet for dstFactor
    };
  };

  public:
    pointStateItem() : m_pointSize(1.0f), m_pointSizeMin(1.0f), m_pointSizeMax(32.0f), m_pointSprite(GL_FALSE), m_options(0) {};
    virtual ~pointStateItem() {};
    virtual void apply();

    friend class stateObserver;
};

//
//
//////////////////////////////////////////////////////////////////////
class passState {
  public:
    passState() {};
    ~passState();

    void setState();


    std::map< std::string, int> m_vRegMap;
    std::map< std::string, int> m_fRegMap;

    std::vector<stateItem*> m_stateList;
};

//
//
//////////////////////////////////////////////////////////////////////
class stateObserver : IObserveFX {
  public:

    stateObserver() : m_state(NULL), m_bs(NULL), m_ds(NULL), m_ss(NULL), m_ps(NULL), m_as(NULL), m_cs(NULL), m_fs(NULL), m_ptS(NULL) {};
    ~stateObserver() {};

    //these are the interface functions used by Ashli
    void setLightState(LightState state, int handle, const char* value);
    void setMaterialState(MaterialState state, int handle, const char* value);
    void setVertexRenderState(VertexRenderState state, int handle, const char* value);
    void setPixelRenderState(PixelRenderState state, int handle, const char* value);
    void setSamplerState(SamplerState state, int handle, const char* value);
    void setVertexShaderState(VertexShaderState state, int handle, const char* value);
    void setPixelShaderState(PixelShaderState state, int handle, const char* value);
    void setTextureState(TextureState state, int handle, const char* value);
    void setTransformState(TransformState state, int handle, const char* value);

    //this is used to configure a pass monitor
    void setPassMonitor( passState *state);
    void finalizePassMonitor();

  protected:

    //These functions parse the values and convert them to GL values
    bool isTrue( const char* value);
    bool isFalse( const char* value);
    GLenum compareFunc( const char* value);
    GLenum blendFactor( const char* value);
    GLenum stencilOp( const char* value);
    GLenum blendOp( const char* value);
    GLenum polyMode( const char* value);

    passState *m_state;

    blendStateItem *m_bs;
    depthStateItem *m_ds;
    stencilStateItem *m_ss;
    primitiveStateItem *m_ps;
    alphaStateItem *m_as;
    colorStateItem *m_cs;
    fogStateItem *m_fs;
    pointStateItem *m_ptS;
};

#endif //OBSERVER_H
