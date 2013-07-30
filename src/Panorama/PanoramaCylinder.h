// PanoramaCylinder.h

#pragma once

#include <string>
#include <vector>
#include <GL/glew.h>
#include "vectortypes.h"

///@brief Constructs and draws a textured cylinder along the y axis centered on the origin.
/// Texture coordinates wrap on x and vary from [0,0.5] along y.
/// Images are loaded from over-under format and a uniform shader variable toggles
/// a 0.5f offset in the y coord for a right eye view(left is the default eye).
class PanoramaCylinder
{
public:
    PanoramaCylinder(const char* pFilename);
    PanoramaCylinder(const char* pFileL, const char* pFileR);
    virtual ~PanoramaCylinder();
    
    virtual void LoadColorTextureFromOverUnderJpeg(const char* pFilename);
    virtual void LoadColorTextureFromJpegPair(const char* pFileL, const char* pFileR);
    virtual void DrawPanoramaGeometry(bool isLeft=true, float vMove=0.0f, float vEyeYaw=0.0f) const;

public:
    GLuint m_panoTexL;
    GLuint m_panoTexR;
    GLuint m_progPanoCylinder;
    GLuint m_cylV;
    GLuint m_cylT;
    GLuint m_cylI;
    GLuint m_capV;
    GLuint m_capT;
    GLuint m_capI;

    unsigned int m_numSlices;
    float m_cylHeight;
    float m_cylRadius;
    float m_pairTweak;
    float m_rollTweak;
    bool  m_manualTexToggle;

    std::vector<float3>       m_cylinderVerts;
    std::vector<float2>       m_cylinderTexs;
    std::vector<unsigned int> m_cylinderIdxs;

    std::vector<float3>       m_capVerts;
    std::vector<float2>       m_capTexs;
    std::vector<unsigned int> m_capIdxs;

protected:
    void _ConstructCylinderGeometry(float coverage = 1.0f);
    void _ConstructCapGeometry();
    void _InitVBOs();
    void _UpdateVBOs();

private: // Disallow default, copy ctor and assignment operator
    PanoramaCylinder();
    PanoramaCylinder(const PanoramaCylinder&);
    PanoramaCylinder& operator=(const PanoramaCylinder&);
};
