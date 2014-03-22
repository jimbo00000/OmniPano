// simple.cpp
// GLFW Skeleton for the basic Oculus Rift/OVR OpenGL app.

#ifdef _WIN32
#  define WINDOWS_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#  include "win/dirent.h"
#else
#  include <dirent.h>
#endif

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>

#include <sstream>
#include <vector>

#include <GL/glew.h>
#include <GL/glfw.h>

#include "OVR.h"
#include "OVR_Shaders.h"
#include "Util/Util_Render_Stereo.h"

#include "vectortypes.h"
#include "GL/ShaderFunctions.h"
#include "Logger.h"
#include "Timer.h"
#include "FBO.h"

#include "OVRkill/OVRkill.h"
#include "PanoramaPatch.h"

#include <iostream>

/// Hope you like global variables!
int running = 0;

#define USE_PATCH

OVRkill g_ok;
#ifdef USE_PATCH
PanoramaPatch* g_pPano = NULL; ///< Initialize AFTER we have a GL context
#else
PanoramaCylinder* g_pPano = NULL; ///< Initialize AFTER we have a GL context
#endif

///
/// VR view parameters
///

// The world RHS coordinate system is defines as follows (as seen in perspective view):
//  Y - Up
//  Z - Back
//  X - Right
const OVR::Vector3f UpVector     (0.0f, 1.0f, 0.0f);
const OVR::Vector3f ForwardVector(0.0f, 0.0f, -1.0f);
const OVR::Vector3f RightVector  (1.0f, 0.0f, 0.0f);

// We start out looking in the positive Z (180 degree rotation).
const float    YawInitial  = 3.141592f;
const float    Sensitivity = 10.0f;
const float    MoveSpeed   = 30.0f; // m/s

const float g_standingHeight = 0.0f;//1.78f; /// From Oculus SDK p.13: 1.78m ~= 5'10"
const float g_crouchingHeight = 0.6f;
OVR::Vector3f EyePos(0.0f, g_standingHeight, 0.0f);
float EyeYaw = YawInitial;
float EyePitch = 0;
float EyeRoll = 0;
float LastSensorYaw = 0;
OVR::Vector3f FollowCamPos(EyePos.x, EyePos.y + 3.0f, EyePos.z + 3.0f);
bool UseFollowCam = false;

/// World modelview matrix
OVR::Matrix4f  View;

/// Viewing parameters fed in from joystick and the HMD
OVR::Quatf  g_joyOrient;
OVR::Quatf  g_hmdOrient;
OVR::Vector3f  GamepadMove, GamepadRotate;
OVR::Vector3f  MouseMove, MouseRotate;
OVR::Vector3f  KeyboardMove, KeyboardRotate;


int g_windowWidth = 1280;
int g_windowHeight = 800;

float g_viewAngle = 45.0; ///< For the no HMD case


// mouse motion internal state
int oldx, oldy, newx, newy;
int which_button = -1;
int modifier_mode = 0;
int last_wheelpos = 0;

int g_keyStates[GLFW_KEY_LAST] = {0};

Timer g_timer;


int currentPano = 0;
void InitPano(int argc, char *argv[]);
std::string datadir = "panos/";
std::vector<std::string> panoFiles;


// OpenGL init:
// Call this once GL context is created.
void OpenGL_initialization()
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}


// Handle mouse clicks
void GLFWCALL clickhandler(int button, int action)
{
    which_button = button;
    glfwGetMousePos(&newx, &newy);
    oldx = newx;
    oldy = newy;
    if (action == GLFW_RELEASE)
    {
        which_button = 0;
    }
    if ((button == GLFW_MOUSE_BUTTON_RIGHT) && (action == GLFW_PRESS))
    {
        InitPano(1, NULL);
    }
}

// Handle mouse drags
void GLFWCALL motionhandler(int x, int y)
{
    const float thresh = 32;

    oldx = newx;
    oldy = newy;
    newx = x;
    newy = y;
    const int mmx = x-oldx;
    const int mmy = y-oldy;
    const float rx = (float)mmx/thresh;
    const float ry = (float)mmy/thresh;
    
    MouseRotate = OVR::Vector3f(0, 0, 0);
    MouseMove   = OVR::Vector3f(0, 0, 0);

    if (glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT))
    {
        MouseRotate = OVR::Vector3f(rx, ry, 0);
    }
    else if (glfwGetMouseButton(GLFW_MOUSE_BUTTON_RIGHT))
    {
        MouseMove   = OVR::Vector3f(rx * rx * (rx > 0 ? 1 : -1),
                                    0,
                                    ry * ry * (ry > 0 ? 1 : -1));
    }
}

void GLFWCALL wheelhandler(int pos)
{
    int delta = pos - last_wheelpos;

    if (g_pPano != NULL)
    {
#ifdef USE_PATCH
        if (delta < 0)
            g_pPano->DecreaseCoverage();
        else if (delta > 0)
            g_pPano->IncreaseCoverage();
#endif
    }

    last_wheelpos = pos;
}

// Handle keyboard events
void GLFWCALL keyhandler(int key, int action)
{
    if ((key > -1) && (key <= GLFW_KEY_LAST))
    {
        g_keyStates[key] = action;
        printf("key ac  %d %d\n", key, action);
    }

    if (key == GLFW_KEY_LSHIFT)
    {
        if (action == GLFW_PRESS)
        {
            EyePos.y = g_crouchingHeight;
        }
        else if (action == GLFW_RELEASE)
        {
            EyePos.y = g_standingHeight;
        }
    }

    if (action != GLFW_PRESS)
    {
        return;
    }

    const float factor = 1.05f;
    const float tweak = 0.001f;
    switch (key)
    {
    case GLFW_KEY_ESC:
        running = GL_FALSE;
        break;

    case GLFW_KEY_F1:
        g_ok.SetDisplayMode(OVRkill::SingleEye);
        UseFollowCam = false;
        break;

    case GLFW_KEY_F2:
        g_ok.SetDisplayMode(OVRkill::Stereo);
        UseFollowCam = false;
        break;

    case GLFW_KEY_F3:
        g_ok.SetDisplayMode(OVRkill::StereoWithDistortion);
        UseFollowCam = false;
        break;

    case GLFW_KEY_F4:
        g_ok.SetDisplayMode(OVRkill::SingleEye);
        UseFollowCam = true;
        break;

    case 'J':
        if (g_pPano != NULL)
            g_pPano->m_cylHeight *= factor;
        break;
    case 'K':
        if (g_pPano != NULL)
            g_pPano->m_cylHeight /= factor;
        break;

    case 'N':
        if (g_pPano != NULL)
            g_pPano->m_cylRadius *= factor;
        break;
    case 'M':
        if (g_pPano != NULL)
            g_pPano->m_cylRadius /= factor;
        break;

    case 'L':
        if (g_pPano != NULL)
            g_pPano->m_manualTexToggle = !g_pPano->m_manualTexToggle;
        break;
        
    case 'Z':
        if (g_pPano != NULL)
        {
            g_pPano->m_pairTweak -= tweak;
            printf("tweak: %f\n", g_pPano->m_pairTweak);
        }
        break;
    case 'X':
        if (g_pPano != NULL)
        {
            g_pPano->m_pairTweak += tweak;
            printf("tweak: %f\n", g_pPano->m_pairTweak);
        }
        break;


    case 'Y':
        if (g_pPano != NULL)
        {
            const float tw = 0.010f;
            g_pPano->m_rollTweak -= tw;
            printf("rolltweak: %f\n", g_pPano->m_rollTweak);
        }
        break;
    case 'U':
        if (g_pPano != NULL)
        {
            const float tw = 0.010f;
            g_pPano->m_rollTweak += tw;
            printf("rolltweak: %f\n", g_pPano->m_rollTweak);
        }
        break;

    case GLFW_KEY_SPACE:
        {
            InitPano(1,NULL);
        }
        break;

    default:
        printf("%d: %c\n", key, (char)key);
        break;
    }

    fflush(stdout);
}



/// Handle input's influence on orientation variables.
void AccumulateInputs(float dt)
{
    // Handle Sensor motion.
    // We extract Yaw, Pitch, Roll instead of directly using the orientation
    // to allow "additional" yaw manipulation with mouse/controller.
    if (g_ok.SensorActive())
    {
        OVR::Quatf    hmdOrient = g_ok.GetOrientation();
        float    yaw = 0.0f;

        hmdOrient.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(&yaw, &EyePitch, &EyeRoll);

        EyeYaw += (yaw - LastSensorYaw);
        LastSensorYaw = yaw;
    }


    // Gamepad rotation.
    EyeYaw -= GamepadRotate.x * dt;

    if (!g_ok.SensorActive())
    {
        // Allow gamepad to look up/down, but only if there is no Rift sensor.
        EyePitch -= GamepadRotate.y * dt;
        EyePitch -= Sensitivity * MouseRotate.y * dt;
        EyeYaw   -= Sensitivity * MouseRotate.x * dt;

        const float maxPitch = ((3.1415f/2)*0.98f);
        if (EyePitch > maxPitch)
            EyePitch = maxPitch;
        if (EyePitch < -maxPitch)
            EyePitch = -maxPitch;
    }
}

/// From the OVR SDK.
void AssembleViewMatrix()
{
    // Rotate and position View Camera, using YawPitchRoll in BodyFrame coordinates.
    // 
    OVR::Matrix4f rollPitchYaw = //OVR::Matrix4f::RotationY(EyeYaw) *
                                 OVR::Matrix4f::RotationX(EyePitch) *
                                 OVR::Matrix4f::RotationZ(EyeRoll);
    OVR::Vector3f up      = rollPitchYaw.Transform(UpVector);
    OVR::Vector3f forward = rollPitchYaw.Transform(ForwardVector);

    // Minimal head modelling.
    float headBaseToEyeHeight     = 0.15f;  // Vertical height of eye from base of head
    float headBaseToEyeProtrusion = 0.09f;  // Distance forward of eye from base of head

    OVR::Vector3f eyeCenterInHeadFrame(0.0f, headBaseToEyeHeight, -headBaseToEyeProtrusion);
    OVR::Vector3f shiftedEyePos = EyePos + rollPitchYaw.Transform(eyeCenterInHeadFrame);
    shiftedEyePos.y -= eyeCenterInHeadFrame.y; // Bring the head back down to original height

    View = OVR::Matrix4f::LookAtRH(shiftedEyePos, shiftedEyePos + forward, up); 

    // This is what transformation would be without head modeling.
    // View = Matrix4f::LookAtRH(EyePos, EyePos + forward, up);

    if (UseFollowCam)
    {
        OVR::Vector3f viewTarget(EyePos);
        OVR::Vector3f viewVector = viewTarget - FollowCamPos;
        View = OVR::Matrix4f::LookAtRH(FollowCamPos, viewVector, up); 
    }
}


/// Handle animations, joystick states and viewing matrix
void timestep(float dt)
{
    AccumulateInputs(dt);
    AssembleViewMatrix();
}


void RenderForOneEye(const OVR::Matrix4f& mview, const OVR::Matrix4f& persp, bool isLeft=true)
{
    if (g_pPano == NULL)
        return;

    glUseProgram(g_pPano->m_progPanoCylinder);
    {
        glUniformMatrix4fv(getUniLoc(g_pPano->m_progPanoCylinder, "mvmtx"), 1, false, &mview.Transposed().M[0][0]);
        glUniformMatrix4fv(getUniLoc(g_pPano->m_progPanoCylinder, "prmtx"), 1, false, &persp.Transposed().M[0][0]);
        
        const float vMove = sin(EyeRoll);
        const float vEyeYaw = -EyeYaw / (2.0f * (float)M_PI);
        g_pPano->DrawPanoramaGeometry(isLeft, vMove, vEyeYaw);
    }
    glUseProgram(0);
}


/// Set up view matrices, then draw scene
void display()
{
    glEnable(GL_DEPTH_TEST);

    g_ok.BindRenderBuffer();
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const int fboWidth = g_ok.GetRenderBufferWidth();
        const int fboHeight = g_ok.GetRenderBufferHeight();
        const int halfWidth = fboWidth/2;
        if (g_ok.GetStereoMode() == OVR::Util::Render::Stereo_LeftRight_Multipass)
        {
            const OVR::HMDInfo& hmd = g_ok.GetHMD();
            // Compute Aspect Ratio. Stereo mode cuts width in half.
            float aspectRatio = float(hmd.HResolution * 0.5f) / float(hmd.VResolution);

            // Compute Vertical FOV based on distance.
            float halfScreenDistance = (hmd.VScreenSize / 2);
            float yfov = 2.0f * atan(halfScreenDistance/hmd.EyeToScreenDistance);

            // Post-projection viewport coordinates range from (-1.0, 1.0), with the
            // center of the left viewport falling at (1/4) of horizontal screen size.
            // We need to shift this projection center to match with the lens center.
            // We compute this shift in physical units (meters) to correct
            // for different screen sizes and then rescale to viewport coordinates.
            float viewCenterValue = hmd.HScreenSize * 0.25f;
            float eyeProjectionShift = viewCenterValue - hmd.LensSeparationDistance * 0.5f;
            float projectionCenterOffset = 4.0f * eyeProjectionShift / hmd.HScreenSize;

            // Projection matrix for the "center eye", which the left/right matrices are based on.
            OVR::Matrix4f projCenter = OVR::Matrix4f::PerspectiveRH(yfov, aspectRatio, 0.3f, 1000.0f);
            OVR::Matrix4f projLeft   = OVR::Matrix4f::Translation(projectionCenterOffset, 0, 0) * projCenter;
            OVR::Matrix4f projRight  = OVR::Matrix4f::Translation(-projectionCenterOffset, 0, 0) * projCenter;
        
            // View transformation translation in world units.
            float halfIPD = hmd.InterpupillaryDistance * 0.5f;
            OVR::Matrix4f viewLeft = OVR::Matrix4f::Translation(halfIPD, 0, 0) * View;
            OVR::Matrix4f viewRight= OVR::Matrix4f::Translation(-halfIPD, 0, 0) * View;

            glViewport(0        ,0,(GLsizei)halfWidth, (GLsizei)fboHeight);
            glScissor (0        ,0,(GLsizei)halfWidth, (GLsizei)fboHeight);
            RenderForOneEye(viewLeft, projLeft, true);

            glViewport(halfWidth,0,(GLsizei)halfWidth, (GLsizei)fboHeight);
            glScissor (halfWidth,0,(GLsizei)halfWidth, (GLsizei)fboHeight);
            RenderForOneEye(viewRight, projRight, false);
        }
        else
        {
            /// Set up our 3D transformation matrices
            /// Remember DX and OpenGL use transposed conventions. And doesn't DX use left-handed coords?
            OVR::Matrix4f mview = View;
            mview = OVR::Matrix4f::Translation(1, 0, -30) * mview;
            OVR::Matrix4f persp = OVR::Matrix4f::PerspectiveRH(g_viewAngle, (float)g_windowWidth/(float)g_windowHeight, 0.004f, 500.0f);

            glViewport(0,0,(GLsizei)fboWidth, (GLsizei)fboHeight);
            RenderForOneEye(mview, persp);
        }
    }
    g_ok.UnBindRenderBuffer();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    OVRkill::PostProcessType post = OVRkill::PostProcess_None;
    RiftDistortionParams  riftDist;
    g_ok.PresentFbo(post, riftDist);

    glfwSwapBuffers();
}

// Window resized event callback
void GLFWCALL reshape(int w, int h)
{
    g_windowWidth = w;
    g_windowHeight = h;

    glViewport(0,0,(GLsizei)w, (GLsizei)h);

    glfwSwapBuffers();
}


/// Scan a directory for jpg files to display and return the list of filenames.
std::vector<std::string> GetFileList(const std::string& datadir)
{
    std::vector<std::string> panoFiles;

    /// Thank you Toni Ronkko for the dirent Windows compatibility layer.
    /// http://stackoverflow.com/questions/612097/how-can-i-get-a-list-of-files-in-a-directory-using-c-or-c
    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(datadir.c_str())) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            std::string filename(ent->d_name);

            /// Check file suffixes
            if (filename.length() > 4)
            {
                std::string suffix = filename.substr(filename.length()-4, 4);
                if (
                    !suffix.compare(".jpg") ||
                    !suffix.compare(".JPG")
                    )
                {
                    printf("%s\n", filename.c_str());
                    std::string fullname = datadir;
                    fullname.append(filename);
                    panoFiles.push_back(fullname);
                }
            }
        }
        closedir(dir);
    }

    return panoFiles;
}

/// Load a panoramic pair from file, preferably over/under but we try to do pairs as well.
void InitPano(int argc, char *argv[])
{
    if (panoFiles.empty())
        return;

    delete g_pPano, g_pPano = NULL;
    display();

    std::string fullFilename(panoFiles[currentPano]);
    ++currentPano;
    currentPano %= panoFiles.size();

    if (argc == 1)
    {
        g_pPano = new PanoramaPatch(fullFilename.c_str());
    }
    else if (argc == 2) ///< Specify over/under image on cmd line
    {
        fullFilename.assign(argv[1]);
        g_pPano = new PanoramaPatch(fullFilename.c_str());
    }
    else if (argc == 3) ///< Specify pair of images on cmd line
    {
        std::string fileL(argv[1]);
        std::string fileR(argv[2]);
        g_pPano = new PanoramaPatch(fileL.c_str(), fileR.c_str());
    }
}


// Initialize then enter the main loop
int main(int argc, char *argv[])
{
    /// Find stereo panoramas checking parent directories.
    const std::string originalDatadir = datadir;
    panoFiles = GetFileList(datadir);
    const int outdepth = 5;
    for (int i=0; i<outdepth; ++i)
    {
        if (panoFiles.empty())
        {
            datadir = "../" + datadir;
            panoFiles = GetFileList(datadir);
        }
    }

    if (panoFiles.empty())
    {
        //running = GL_FALSE;
        std::ostringstream oss;
        oss << std::endl
            << "No files found from "
            << originalDatadir
            << " to "
            << datadir
            << ", so we are exiting."
            << std::endl
            << "Throw some stereo panoramas in there!"
            << std::endl;

        std::cerr << oss.str().c_str();
        LOG_INFO("%s", oss.str().c_str());

#ifdef _WIN32
        MessageBox(0, oss.str().c_str(), "No pano files found", 0);
        ///@note This will prevent console output from disappearing with the terminal on Windows,
        /// but makes for an unpleasant experience when run with the Rift on.
        //char ch;
        //std::cin >> ch;
#endif

        exit(0);
    }


    bool fullScreen = false;

#ifdef _WIN32
    if (!IsDebuggerPresent()) ///< Ctrl-F5 to run in Rift mode,  just F5 for windowed
#endif
    {
        fullScreen = true;
    }

    g_ok.InitOVR();
    
    if (fullScreen)
    {
        g_ok.SetDisplayMode(OVRkill::StereoWithDistortion);
    }
    else
    {
        g_ok.SetDisplayMode(OVRkill::SingleEye);
    }


    LOG_INFO("Initializing Glfw and window @ %d x %d", g_windowWidth, g_windowHeight);
    glfwInit();

    if (!glfwOpenWindow(
        g_windowWidth, g_windowHeight,
        8,8,8,8,
        0,8,
        fullScreen ? 
            GLFW_FULLSCREEN :
            GLFW_WINDOW
        ))
    {
        glfwTerminate();
        return 0;
    }

    glfwSetWindowTitle        ("Oculus GLFW Skeleton");
    glfwSetKeyCallback        (keyhandler);
    glfwSetWindowSizeCallback (reshape);
    glfwSetMouseButtonCallback(clickhandler);
    glfwSetMousePosCallback   (motionhandler);
    glfwSetMouseWheelCallback (wheelhandler);


    /// Create window and context before calling glewInit
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        LOG_INFO("Glew error... %s", glewGetErrorString(err));
        printf("Glew error... %s\n", glewGetErrorString(err));
    }

    OpenGL_initialization();
    LOG_INFO("Initializing shaders.");
    g_ok.CreateShaders();
    g_ok.CreateRenderBuffer(1.71f);


    InitPano(argc, argv);

    /// Main loop
    running = GL_TRUE;
    g_timer.reset();
    while (running)
    {
        float dt = (float)g_timer.seconds();
        timestep(dt);
        g_timer.reset();
        display();
        running = running && glfwGetWindowParam(GLFW_OPENED);
    }

    g_ok.DestroyOVR();

    glfwTerminate();
    return 0;
}
