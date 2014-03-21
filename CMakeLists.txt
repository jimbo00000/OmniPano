CMAKE_MINIMUM_REQUIRED( VERSION 2.6 )
PROJECT( OmniPano )

INCLUDE(cmake_modules/HardcodeShaders.cmake)

#
# Platform-dependent section
#
IF( WIN32 )
    #
    # Custom Windows include and link dirs for my machine:
    #
    SET( LIBS_HOME "C:/lib" )

    SET( GLFW_ROOT "${LIBS_HOME}/glfw-2.7.8.bin.WIN32" )
    INCLUDE_DIRECTORIES( "${GLFW_ROOT}/include" )
    LINK_DIRECTORIES   ( "${GLFW_ROOT}/lib-msvc100" )

    SET( GLEW_ROOT "${LIBS_HOME}/glew" )
    INCLUDE_DIRECTORIES( "${GLEW_ROOT}/include" )
    LINK_DIRECTORIES   ( "${GLEW_ROOT}/lib" )
    #ADD_DEFINITIONS( -DGLEW_STATIC )
    
    SET( OCULUSSDK_ROOT "${LIBS_HOME}/OculusSDK-0.2.2" )
    INCLUDE_DIRECTORIES( "${OCULUSSDK_ROOT}/LibOVR/Include" )
    INCLUDE_DIRECTORIES( "${OCULUSSDK_ROOT}/LibOVR/Src" )
    LINK_DIRECTORIES   ( "${OCULUSSDK_ROOT}/LibOVR/Lib/Win32" )

    SET( PLATFORM_LIBS
        glfw.lib
        opengl32.lib
        glu32.lib
        glew32.lib 
        libovr.lib
        winmm.lib
        )

    SOURCE_GROUP( "Build" FILES CMakeLists.txt )

ELSEIF( UNIX )
    ADD_DEFINITIONS( -D_LINUX )
    SET( LIBS_HOME "~/lib" )
    
    SET( OCULUSSDK_ROOT "${LIBS_HOME}/OculusSDK" )
    INCLUDE_DIRECTORIES( "${OCULUSSDK_ROOT}/LibOVR/Include" )
    INCLUDE_DIRECTORIES( "${OCULUSSDK_ROOT}/LibOVR/Src" )
    #LINK_DIRECTORIES   ( "${OCULUSSDK_ROOT}/LibOVR/Lib/Linux/Debug/x86_64" )
    LINK_DIRECTORIES   ( "${OCULUSSDK_ROOT}/LibOVR/Lib/Linux/Release/x86_64/" )

    SET( PLATFORM_LIBS
        -lovr
        -lGLEW
        -lGLU -lglfw
        -pthread
        -ludev
        -lrt
        -lXinerama
        -lXrandr
        -lXxf86vm
        -lXi
        ${ANT_LIBS}
        )

ENDIF()


#
# Platform-independent section
#
FILE( GLOB_RECURSE UTIL_SOURCE_FILES
    src/utils/*.cpp
    src/utils/*.h
    )

FILE( GLOB_RECURSE VECTORMATH_SOURCE_FILES
    src/vectormath/*.cpp
    src/vectormath/*.h
    )

FILE( GLOB_RECURSE OVRKILL_SOURCE_FILES
    src/OVRkill/*.cpp
    src/OVRkill/*.h
    )

FILE( GLOB_RECURSE PANORAMA_SOURCE_FILES
    src/Panorama/*.cpp
    src/Panorama/*.h
    )

FILE( GLOB_RECURSE JPEG_COMPRESSOR_SOURCE_FILES
    src/jpeg-compressor/*.cpp
    src/jpeg-compressor/*.h
    )

INCLUDE_DIRECTORIES("src")
INCLUDE_DIRECTORIES("src/utils")
INCLUDE_DIRECTORIES("src/utils/GL")
INCLUDE_DIRECTORIES("src/vectormath")
INCLUDE_DIRECTORIES("src/OVRkill")
INCLUDE_DIRECTORIES("src/Panorama")
INCLUDE_DIRECTORIES("src/jpeg-compressor")

ADD_LIBRARY( VectorMath ${VECTORMATH_SOURCE_FILES} )
ADD_LIBRARY( OVRkill    ${OVRKILL_SOURCE_FILES} )
ADD_LIBRARY( Util       ${UTIL_SOURCE_FILES} )
ADD_LIBRARY( Panorama   ${PANORAMA_SOURCE_FILES} )
ADD_LIBRARY( Jpeg       ${JPEG_COMPRESSOR_SOURCE_FILES} )

ADD_EXECUTABLE( ${PROJECT_NAME}
    ${SOURCE_FILES}
    src/skeleton/simple_glfw_skeleton.cpp
    )

TARGET_LINK_LIBRARIES( ${PROJECT_NAME}
    OVRkill
    Util
    VectorMath
    Panorama
    Jpeg
    ${PLATFORM_LIBS}
    )
