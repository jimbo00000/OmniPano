// StereoPanoramaScene.cpp

#include "StereoPanoramaScene.h"
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define _USE_MATH_DEFINES
#include <math.h>

StereoPanoramaScene::StereoPanoramaScene()
: m_basic()
, m_triCount(0)
{
}

StereoPanoramaScene::~StereoPanoramaScene()
{
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

#if 1
            // Scale radius by height with fattest part at y == 0.5
            const float x = t - 0.5f;
            const float sphereFactor = 2.0f * sqrt(0.25f - fabs(x*x));
            glm::vec3 sphericalXzvec(xzvec);
            sphericalXzvec.x *= sphereFactor;
            sphericalXzvec.z *= sphereFactor;
            verts.push_back(sphericalXzvec);
#else
            verts.push_back(xzvec);
#endif

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
    m_basic.AddVbo("vPosition", vertVbo);
    glBindBuffer(GL_ARRAY_BUFFER, vertVbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(glm::vec3), &verts[0], GL_STATIC_DRAW);
    glVertexAttribPointer(m_basic.GetAttrLoc("vPosition"), 3, GL_FLOAT, GL_FALSE, 0, NULL);

    GLuint colVbo = 0;
    glGenBuffers(1, &colVbo);
    m_basic.AddVbo("vColor", colVbo);
    glBindBuffer(GL_ARRAY_BUFFER, colVbo);
    glBufferData(GL_ARRAY_BUFFER, cols.size()*sizeof(glm::vec2), &cols[0], GL_STATIC_DRAW);
    glVertexAttribPointer(m_basic.GetAttrLoc("vColor"), 2, GL_FLOAT, GL_FALSE, 0, NULL);

    glEnableVertexAttribArray(m_basic.GetAttrLoc("vPosition"));
    glEnableVertexAttribArray(m_basic.GetAttrLoc("vColor"));

    GLuint quadVbo = 0;
    glGenBuffers(1, &quadVbo);
    m_basic.AddVbo("elements", quadVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadVbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, inds.size()*sizeof(GLuint), &inds[0], GL_STATIC_DRAW);
}

void StereoPanoramaScene::initGL()
{
    m_basic.initProgram("basic");
    m_basic.bindVAO();
    _InitCylinderAttributes();
    glBindVertexArray(0);
}

void StereoPanoramaScene::DrawScene(
    const glm::mat4& modelview,
    const glm::mat4& projection,
    const glm::mat4& object) const
{
    glUseProgram(m_basic.prog());
    {
        glUniformMatrix4fv(m_basic.GetUniLoc("mvmtx"), 1, false, glm::value_ptr(modelview));
        glUniformMatrix4fv(m_basic.GetUniLoc("prmtx"), 1, false, glm::value_ptr(projection));

        m_basic.bindVAO();
        glDisable(GL_CULL_FACE);
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
