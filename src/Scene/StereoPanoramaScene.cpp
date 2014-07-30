// StereoPanoramaScene.cpp

#include "StereoPanoramaScene.h"
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define _USE_MATH_DEFINES
#include <math.h>

#include "jpgd.h"

StereoPanoramaScene::StereoPanoramaScene()
: m_panoShader()
, m_triCount(0)
, m_panoTexL(0)
, m_panoTexR(0)
, m_useSphericalGeometry(false)
{
}

StereoPanoramaScene::~StereoPanoramaScene()
{
    glDeleteTextures(1, &m_panoTexL);
    glDeleteTextures(1, &m_panoTexR);
}



/// Call gluBuild2DMipmaps to create a texture from half the data buffer(over/under format)
void UploadBoundTex(int width, int height, int comps, unsigned char* pData, bool isLeft, bool isOverUnder)
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
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

void StereoPanoramaScene::LoadMonoPanoFromJpeg(const char* pFilename)
{
    if (pFilename == NULL)
        return;

    int width = 0;
    int height = 0;
    int comps = 3;
    unsigned char* pData = jpgd::decompress_jpeg_image_from_file(
        pFilename,
        &width,
        &height,
        &comps,
        comps);

    glDeleteTextures(1, &m_panoTexL);
    glDeleteTextures(1, &m_panoTexR);
    m_panoTexL = 0;
    m_panoTexR = 0;

    glGenTextures(1, &m_panoTexL);
    glBindTexture(GL_TEXTURE_2D, m_panoTexL);
    UploadBoundTex(width, height, comps, pData, true, false);
    m_panoTexR = m_panoTexL;

    m_useSphericalGeometry = false;

    glBindTexture(GL_TEXTURE_2D, 0);

    delete [] pData;
}

/// Load image data from a Jpeg into texture.
///@param pFilename Filename of the image to load(in over/under Jpeg format)
void StereoPanoramaScene::LoadStereoPanoFromOverUnderJpeg(const char* pFilename)
{
    if (pFilename == NULL)
        return;

    int width = 0;
    int height = 0;
    int comps = 3;
    unsigned char* pData = jpgd::decompress_jpeg_image_from_file(
        pFilename,
        &width,
        &height,
        &comps,
        comps);

    glDeleteTextures(1, &m_panoTexL);
    glDeleteTextures(1, &m_panoTexR);
    m_panoTexL = 0;
    m_panoTexR = 0;

    glGenTextures(1, &m_panoTexL);
    glBindTexture(GL_TEXTURE_2D, m_panoTexL);
    UploadBoundTex(width, height, comps, pData, true, true);

    glGenTextures(1, &m_panoTexR);
    glBindTexture(GL_TEXTURE_2D, m_panoTexR);
    UploadBoundTex(width, height, comps, pData, false, true);

    m_useSphericalGeometry = true;

    glBindTexture(GL_TEXTURE_2D, 0);

    delete [] pData;
}

void StereoPanoramaScene::LoadColorTextureFromJpegPair(const char* pFileL, const char* pFileR)
{
    if (pFileL == NULL)
        return;
    if (pFileR == NULL)
        return;

    {
        int width = 0;
        int height = 0;
        int comps = 1;
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
        int width = 0;
        int height = 0;
        int comps = 1;
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


///@brief Form a cylindrical strip of quads, 4 verts per face.
void ConstructCylinderGeometry(
    std::vector<glm::vec3>& verts,
    std::vector<glm::vec2>& cols,
    std::vector<unsigned int>& inds,
    float coverage)
{
    const float height = 5.0f;
    const float radius = 8.0f;
    const int slices = 64;
    const int stacks = 32;
    const float fPi = static_cast<float>(M_PI);

    for (int i=0; i<=slices; ++i)
    {
        float phase = coverage * 2.0f * fPi * static_cast<float>(i) / static_cast<float>(slices);
        // Align seam directly to the back
        phase -= 0.5f * coverage * 2.0f * fPi;
        phase += fPi;

        // y is up, center at origin
        glm::vec3 xzvec = glm::vec3(
            radius * sin(phase),
            0.0f,
            radius * cos(phase)
            );

        for (int j=0; j<=stacks; ++j)
        {
            const float hmin = -height;
            const float hmax = height;
            const float t = static_cast<float>(j) / static_cast<float>(stacks);
            const float h = (1-t)*hmin + t*hmax;
            xzvec.y = h;
            verts.push_back(xzvec);

            const float tscale = 1.0f;
            glm::vec2 c(
                1.0f - static_cast<float>(i) / static_cast<float>(slices),
                tscale * (1-t)
            );
            cols.push_back(c);
        }

        if (i > 0)
        {
            for (int j=0; j<stacks; ++j)
            {
                const int s = (stacks+1);
                inds.push_back(s* i +j );
                inds.push_back(s*(i-1)+j );
                inds.push_back(s*(i-1)+j+1);
                inds.push_back(s*(i )+j+1);
            }
        }
    }
}

void StereoPanoramaScene::_InitCylinderAttributes()
{
    std::vector<glm::vec3> verts;
    std::vector<glm::vec2> cols;
    std::vector<unsigned int> inds;

    ConstructCylinderGeometry(
        verts,
        cols,
        inds,
        1.0f);

    m_triCount = inds.size();

    GLuint vertVbo = 0;
    glGenBuffers(1, &vertVbo);
    m_panoShader.AddVbo("vPosition", vertVbo);
    glBindBuffer(GL_ARRAY_BUFFER, vertVbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(glm::vec3), &verts[0], GL_STATIC_DRAW);
    glVertexAttribPointer(m_panoShader.GetAttrLoc("vPosition"), 3, GL_FLOAT, GL_FALSE, 0, NULL);

    GLuint texVbo = 0;
    glGenBuffers(1, &texVbo);
    m_panoShader.AddVbo("vTex", texVbo);
    glBindBuffer(GL_ARRAY_BUFFER, texVbo);
    glBufferData(GL_ARRAY_BUFFER, cols.size()*sizeof(glm::vec2), &cols[0], GL_STATIC_DRAW);
    glVertexAttribPointer(m_panoShader.GetAttrLoc("vTex"), 2, GL_FLOAT, GL_FALSE, 0, NULL);

    glEnableVertexAttribArray(m_panoShader.GetAttrLoc("vPosition"));
    glEnableVertexAttribArray(m_panoShader.GetAttrLoc("vTex"));

    GLuint quadVbo = 0;
    glGenBuffers(1, &quadVbo);
    m_panoShader.AddVbo("elements", quadVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadVbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, inds.size()*sizeof(GLuint), &inds[0], GL_STATIC_DRAW);
}

void StereoPanoramaScene::initGL()
{
    m_panoShader.initProgram("pano");
    m_panoShader.bindVAO();
    _InitCylinderAttributes();
    glBindVertexArray(0);
}

void StereoPanoramaScene::DrawScene(
    const glm::mat4& modelview,
    const glm::mat4& projection,
    const glm::mat4& object) const
{
    glUseProgram(m_panoShader.prog());
    {
        glUniformMatrix4fv(m_panoShader.GetUniLoc("mvmtx"), 1, false, glm::value_ptr(modelview));
        glUniformMatrix4fv(m_panoShader.GetUniLoc("prmtx"), 1, false, glm::value_ptr(projection));

        m_panoShader.bindVAO();
        glDisable(GL_CULL_FACE);

        const float tweak = glm::value_ptr(projection)[8];
        const bool left = tweak < 0.0f;
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, left ? m_panoTexL : m_panoTexR);
        glUniform1i(m_panoShader.GetUniLoc("texImage"), 0);

        glUniform1i(m_panoShader.GetUniLoc("useSphereGeometry"), m_useSphericalGeometry ? 1 : 0);

        glDrawElements(GL_QUADS,
                       m_triCount,
                       GL_UNSIGNED_INT,
                       0);
        glBindVertexArray(0);
    }
    glUseProgram(0);
}

void StereoPanoramaScene::RenderForOneEye(const float* pMview, const float* pPersp, const float* pObject) const
{
    if (m_bDraw == false)
        return;

    const glm::mat4 modelview = glm::make_mat4(pMview);
    const glm::mat4 projection = glm::make_mat4(pPersp);

    glm::mat4 object = glm::mat4(1.0f);
    if (pObject != NULL)
        object = glm::make_mat4(pObject);

    DrawScene(modelview, projection, object);
}

void StereoPanoramaScene::timestep(float dt)
{
}
