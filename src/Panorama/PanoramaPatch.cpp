// PanoramaPatch.cpp

#include "PanoramaPatch.h"

///@param pFilename Filename of the image to load(in over/under format)
PanoramaPatch::PanoramaPatch(const char* pFilename)
 : PanoramaCylinder(pFilename)
 , m_cylCoverage(1.0f)
{
    _ConstructPatchGeometry();
}

PanoramaPatch::PanoramaPatch(const char* pFileL, const char* pFileR)
 : PanoramaCylinder(pFileL, pFileR)
 , m_cylCoverage(1.0f)
{
    _ConstructPatchGeometry();
}

PanoramaPatch::~PanoramaPatch()
{
}

///@param isLeft Set to true if rendering for left eye, false for right eye
void PanoramaPatch::DrawPanoramaGeometry(bool isLeft, float vMove, float vEyeYaw) const
{
    PanoramaCylinder::DrawPanoramaGeometry(isLeft, vMove, vEyeYaw);
}

void PanoramaPatch::_ConstructPatchGeometry()
{
    m_cylinderVerts.clear();
    m_cylinderTexs.clear();
    m_cylinderIdxs.clear();

    _ConstructCylinderGeometry(m_cylCoverage);
    _UpdateVBOs();
}

void PanoramaPatch::IncreaseCoverage()
{
    m_cylCoverage *= 1.1f;
    m_cylCoverage = std::min(m_cylCoverage, 1.0f);
    _ConstructPatchGeometry();
}

void PanoramaPatch::DecreaseCoverage()
{
    m_cylCoverage /= 1.1f;
    m_cylCoverage = std::max(m_cylCoverage, 0.125f);
    _ConstructPatchGeometry();
}
