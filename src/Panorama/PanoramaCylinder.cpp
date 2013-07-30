// PanoramaCylinder.cpp

#ifdef _WIN32
#  define WINDOWS_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#endif

#define _USE_MATH_DEFINES
#include <math.h>

#include "PanoramaCylinder.h"
#include "Logger.h"
#include "GL/ShaderFunctions.h"
#include "VectorMath.h"

#include "jpgd.h"

#include <vector>
#include <GL/glew.h>


///@param pFilename Filename of the image to load(in over/under format)
PanoramaCylinder::PanoramaCylinder(const char* pFilename)
: m_panoTexL(0)
, m_panoTexR(0)
, m_progPanoCylinder(0)
, m_cylV(0)
, m_cylT(0)
, m_cylI(0)
, m_capV(0)
, m_capT(0)
, m_capI(0)
, m_numSlices(64)
, m_cylHeight(5.0f)
, m_cylRadius(8.0f)
, m_pairTweak(0.016f)
, m_rollTweak(0.0f)
, m_manualTexToggle(false)
, m_cylinderVerts()
, m_cylinderTexs()
, m_cylinderIdxs()
, m_capVerts()
, m_capTexs()
, m_capIdxs()
{
    LoadColorTextureFromOverUnderJpeg(pFilename);

    m_progPanoCylinder = makeShaderByName("panocylinder");
    _ConstructCylinderGeometry();
    _ConstructCapGeometry();
    _InitVBOs();
}


///@param pFileL Filename of the left image to load
///@param pFileR Filename of the right image to load
PanoramaCylinder::PanoramaCylinder(const char* pFileL, const char* pFileR)
: m_panoTexL(0)
, m_panoTexR(0)
, m_progPanoCylinder(0)
, m_cylV(0)
, m_cylT(0)
, m_cylI(0)
, m_capV(0)
, m_capT(0)
, m_capI(0)
, m_numSlices(16)
, m_cylHeight(5.0f)
, m_cylRadius(8.0f)
, m_pairTweak(0.016f)
, m_rollTweak(0.0f)
, m_manualTexToggle(false)
, m_cylinderVerts()
, m_cylinderTexs()
, m_cylinderIdxs()
, m_capVerts()
, m_capTexs()
, m_capIdxs()
{
    LoadColorTextureFromJpegPair(pFileL, pFileR);

    m_progPanoCylinder = makeShaderByName("panocylinder");
    _ConstructCylinderGeometry();
    _ConstructCapGeometry();
    _InitVBOs();
}

PanoramaCylinder::~PanoramaCylinder()
{
    glDeleteTextures(1, &m_panoTexL);
    glDeleteTextures(1, &m_panoTexR);
    glDeleteProgram(m_progPanoCylinder);
    glDeleteBuffers(1, &m_cylV);
    glDeleteBuffers(1, &m_cylT);
    glDeleteBuffers(1, &m_cylI);
    glDeleteBuffers(1, &m_capV);
    glDeleteBuffers(1, &m_capT);
    glDeleteBuffers(1, &m_capI);
}

/// Call gluBuild2DMipmaps to create a texture from half the data buffer(over/under format)
void UploadBoundTex(int width, int height, int comps, unsigned char* pData, bool isLeft, bool isOverUnder)
{
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    GLint numChannels = 1;
    GLenum format = GL_LUMINANCE;
    if (comps == 3)
    {
        numChannels = 3;
        format = GL_RGB;
    }

    const unsigned char* pDataStart = pData;
    if (isOverUnder && !isLeft)
    {
        pDataStart += numChannels * width * height / 2;
    }

    GLsizei h = height;
    if (isOverUnder)
    {
        h /= 2;
    }
    gluBuild2DMipmaps(
        GL_TEXTURE_2D,
        numChannels,
        width,
        h,
        format,
        GL_UNSIGNED_BYTE,
        pDataStart);
}

/// Load image data from a Jpeg into texture.
///@param pFilename Filename of the image to load(in over/under Jpeg format)
void PanoramaCylinder::LoadColorTextureFromOverUnderJpeg(const char* pFilename)
{
    if (pFilename == NULL)
        return;

    int width  = 0;
    int height = 0;
    int comps  = 3;
    unsigned char* pData = jpgd::decompress_jpeg_image_from_file(
        pFilename,
        &width,
        &height,
        &comps,
        comps);

    glDeleteTextures(1, &m_panoTexL);
    glDeleteTextures(1, &m_panoTexR);

    glGenTextures(1, &m_panoTexL);
    glBindTexture(GL_TEXTURE_2D, m_panoTexL);
    UploadBoundTex(width, height, comps, pData, true, true);

    glGenTextures(1, &m_panoTexR);
    glBindTexture(GL_TEXTURE_2D, m_panoTexR);
    UploadBoundTex(width, height, comps, pData, false, true);

    glBindTexture(GL_TEXTURE_2D, 0);

    delete [] pData;
}

void PanoramaCylinder::LoadColorTextureFromJpegPair(const char* pFileL, const char* pFileR)
{
    if (pFileL == NULL)
        return;
    if (pFileR == NULL)
        return;

    {
        int width  = 0;
        int height = 0;
        int comps  = 1;
        unsigned char* pData = jpgd::decompress_jpeg_image_from_file(
            pFileL,
            &width,
            &height,
            &comps,
            comps);

        glDeleteTextures(1, &m_panoTexL);

        glGenTextures(1, &m_panoTexL);
        glBindTexture(GL_TEXTURE_2D, m_panoTexL);
        UploadBoundTex(width, height, comps, pData, true, false);
        delete [] pData;
    }

    {
        int width  = 0;
        int height = 0;
        int comps  = 1;
        unsigned char* pData = jpgd::decompress_jpeg_image_from_file(
            pFileR,
            &width,
            &height,
            &comps,
            comps);

        glDeleteTextures(1, &m_panoTexR);
        glGenTextures(1, &m_panoTexR);
        glBindTexture(GL_TEXTURE_2D, m_panoTexR);
        UploadBoundTex(width, height, comps, pData, false, false);
        delete [] pData;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

/// Form a cylindrical strip of quads, 4 verts per face.
void PanoramaCylinder::_ConstructCylinderGeometry(float coverage)
{
    std::vector<float3>&       verts = m_cylinderVerts;
    std::vector<float2>&       cols  = m_cylinderTexs;
    std::vector<unsigned int>& inds  = m_cylinderIdxs;

    const int slices = m_numSlices;
    const int stacks = 1;
    const float height = m_cylHeight;
    const float radius = m_cylRadius;

    for (int i=0; i<=slices; ++i)
    {
        float phase = coverage * 2.0f * (float)M_PI * (float)i / (float)slices;
        /// Align seam directly to the back
        phase -= 0.5f * coverage * 2.0f * (float)M_PI;
        phase += (float)M_PI;

        /// y is up, center at origin
        float3 xzvec = {
            radius * sin(phase),
            0.0f,
            radius * cos(phase)
        };

        for (int j=0; j<=stacks; ++j)
        {
            float hmin = -height;
            float hmax = height;
            float t = (float)j/(float)stacks;
            float h = (1-t)*hmin + t*hmax;
            xzvec.y = h;
            verts.push_back(xzvec);

            float tscale = 1.0f;
            float2 c = {
                1-(float)i / (float)slices,
                tscale * (1-t),
            };
            cols.push_back(c);
        }

        if (i > 0)
        {
            for (int j=0; j<stacks; ++j)
            {
                const int s = (stacks+1);
                inds.push_back(s* i   +j  );
                inds.push_back(s*(i-1)+j  );
                inds.push_back(s*(i-1)+j+1);
                inds.push_back(s*(i  )+j+1);
            }
        }
    }
}

/// Form a triangle fan about the center of the cylinder's top circle.
void PanoramaCylinder::_ConstructCapGeometry()
{
    std::vector<float3>&       verts = m_capVerts;
    std::vector<float2>&       cols  = m_capTexs;
    std::vector<unsigned int>& inds  = m_capIdxs;

    const int slices = m_numSlices;
    const int stacks = 1;
    const float height = m_cylHeight;
    const float radius = m_cylRadius;

    for (int i=0; i<=slices; ++i)
    {
        const float phase = 2.0f * (float)M_PI * (float)i / (float)slices;

        /// y is up, center at origin
        float3 xzvec = {
            radius * sin(phase),
            0.0f,
            radius * cos(phase)
        };

        /// Draw a cap segment on top of the cylinder, all textured at the top value
        float3 topCenter = {0, height, 0};
        float3 topVert = {
            xzvec.x,
            height,
            xzvec.z
        };
        verts.push_back(topVert);
        verts.push_back(topCenter);

        /// Bump texture addressing by ~1px
        const float epsilon = 1.0f / (float)1696; ///<@todo Depend on y resolution of image
        float2 c = {
            1-(float)i / (float)slices,
            0.0,// + epsilon,
        };
        cols.push_back(c);
        cols.push_back(c);

        if (i > 0)
        {
            const int s = 2;
            inds.push_back(s* i     );
            inds.push_back(s*(i-1)  );
            inds.push_back(s*(i-1)+1);
            //inds.push_back(s*(i  )+1);
        }
    }
}

///@param isLeft Set to true if rendering for left eye, false for right eye
void PanoramaCylinder::DrawPanoramaGeometry(bool isLeft, float vMove, float vEyeYaw) const
{
    glBindBuffer(GL_ARRAY_BUFFER, m_cylV);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, m_cylT);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    bool left = isLeft;
    if (m_manualTexToggle)
        left = !left;

    glActiveTexture(0);
    glBindTexture(GL_TEXTURE_2D, left ? m_panoTexL : m_panoTexR);
    glUniform1i(getUniLoc(m_progPanoCylinder, "texImage"), 0);

    glUniform2f(getUniLoc(m_progPanoCylinder, "texOff"),
        left? 0 : m_pairTweak,
        0.0f );

    /// This was originally an attempt to try to compensate for roll along the forward axis,
    /// but it's not clear that such an idea is even possible with just a stereo pair.
    const float vAmt = -m_rollTweak;
    glUniform1f(getUniLoc(m_progPanoCylinder, "vMove"),
        left? 0 : vAmt*vMove );

    glUniform1f(getUniLoc(m_progPanoCylinder, "vEyeYaw"), vEyeYaw );

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDrawElements(GL_QUADS,
                   m_cylinderIdxs.size(),
                   GL_UNSIGNED_INT,
                   &m_cylinderIdxs[0]);

    if (!m_capVerts.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_capV);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, m_capT);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glDrawElements(GL_TRIANGLES,
                       m_capIdxs.size(),
                       GL_UNSIGNED_INT,
                       &m_capIdxs[0]);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}

///@brief Be sure to call this after _ConstructCylinderGeometry and _ConstructCapGeometry.
void PanoramaCylinder::_InitVBOs()
{
    glGenBuffers(1, &m_cylV);
    glGenBuffers(1, &m_cylT);
    ///@todo Using GL buffers for index data does not appear to work on GTX285
    //glGenBuffers(1, &m_cylI);

    glGenBuffers(1, &m_capV);
    glGenBuffers(1, &m_capT);

    _UpdateVBOs();
}

void PanoramaCylinder::_UpdateVBOs()
{
    glBindBuffer(GL_ARRAY_BUFFER, m_cylV);
    glBufferData(GL_ARRAY_BUFFER, m_cylinderVerts.size()*sizeof(float3), &m_cylinderVerts[0].x, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, m_cylT);
    glBufferData(GL_ARRAY_BUFFER, m_cylinderTexs.size()*sizeof(float2), &m_cylinderTexs[0].x, GL_STATIC_DRAW);
    //glBindBuffer(GL_ARRAY_BUFFER, m_cylI);
    //glBufferData(GL_ARRAY_BUFFER, m_cylinderIdxs.size()*sizeof(unsigned int), &m_cylinderIdxs[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_capV);
    glBufferData(GL_ARRAY_BUFFER, m_capVerts.size()*sizeof(float3), &m_capVerts[0].x, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, m_capT);
    glBufferData(GL_ARRAY_BUFFER, m_capTexs.size()*sizeof(float2), &m_capTexs[0].x, GL_STATIC_DRAW);
}
