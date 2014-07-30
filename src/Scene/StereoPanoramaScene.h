// StereoPanoramaScene.h

#pragma once

#ifdef _WIN32
#  define WINDOWS_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#endif
#include <stdlib.h>
#include <GL/glew.h>

#include <glm/glm.hpp>

#include "IScene.h"
#include "ShaderWithVariables.h"

///@brief Display stereo panorama pairs on a cylindrical "screen" in space.
class StereoPanoramaScene : public IScene
{
public:
    StereoPanoramaScene();
    virtual ~StereoPanoramaScene();

    virtual void LoadMonoPanoFromJpeg(const char* pFilename);
    virtual void LoadStereoPanoFromOverUnderJpeg(const char* pFilename);
    virtual void LoadColorTextureFromJpegPair(const char* pFileL, const char* pFileR);

    virtual void initGL();
    virtual void timestep(float dt);
    virtual void RenderForOneEye(const float* pMview, const float* pPersp, const float* pObject=NULL) const;

protected:
    void _InitCylinderAttributes();
    void DrawScene(
        const glm::mat4& modelview,
        const glm::mat4& projection,
        const glm::mat4& object) const;

    ShaderWithVariables m_basic;
    GLuint m_triCount;
    GLuint m_panoTexL;
    GLuint m_panoTexR;

private: // Disallow copy ctor and assignment operator
    StereoPanoramaScene(const StereoPanoramaScene&);
    StereoPanoramaScene& operator=(const StereoPanoramaScene&);
};
