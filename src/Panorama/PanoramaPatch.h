// PanoramaPatch.h

#pragma once

#include "PanoramaCylinder.h"

///@brief Constructs and draws a textured cylinder along the y axis centered on the origin.
/// Texture coordinates wrap on x and vary from [0,0.5] along y.
/// Images are loaded from over-under format and a uniform shader variable toggles
/// a 0.5f offset in the y coord for a right eye view(left is the default eye).
class PanoramaPatch : public PanoramaCylinder
{
public:
    PanoramaPatch(const char* pFilename);
    PanoramaPatch(const char* pFileL, const char* pFileR);
    virtual ~PanoramaPatch();
    
    virtual void DrawPanoramaGeometry(bool isLeft=true, float vMove=0.0f, float vEyeYaw=0.0f) const;

    virtual void IncreaseCoverage();
    virtual void DecreaseCoverage();

protected:
    void _ConstructPatchGeometry();

    float m_cylCoverage;

private: // Disallow default, copy ctor and assignment operator
    PanoramaPatch();
    PanoramaPatch(const PanoramaPatch&);
    PanoramaPatch& operator=(const PanoramaPatch&);
};
