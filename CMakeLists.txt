cmake_minimum_required(VERSION 3.16)
project(QMLPlayerFFmpeg)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt and FFmpeg installation directories
set(QT_INSTALL_DIR "D:/software/qt/6.9.2/mingw_64" CACHE PATH "Qt6 installation directory")
set(FFMPEG_INSTALL_DIR "D:/software/ffmpeg" CACHE PATH "FFmpeg installation directory")

# Configure Qt and FFmpeg paths
set(CMAKE_PREFIX_PATH ${QT_INSTALL_DIR} ${FFMPEG_INSTALL_DIR} ${CMAKE_PREFIX_PATH})

# Enable Qt automations
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Quick OpenGL Qml QuickControls2 Multimedia)

# Find FFmpeg headers and libraries using configured install directory
find_path(AVFORMAT_INCLUDE_DIR 
    NAMES libavformat/avformat.h
    PATHS ${FFMPEG_INSTALL_DIR}
    PATH_SUFFIXES include include/ffmpeg
    DOC "FFmpeg avformat include directory"
)

find_path(AVCODEC_INCLUDE_DIR 
    NAMES libavcodec/avcodec.h
    PATHS ${FFMPEG_INSTALL_DIR}
    PATH_SUFFIXES include include/ffmpeg
    DOC "FFmpeg avcodec include directory"
)

find_path(AVUTIL_INCLUDE_DIR 
    NAMES libavutil/avutil.h
    PATHS ${FFMPEG_INSTALL_DIR}
    PATH_SUFFIXES include include/ffmpeg
    DOC "FFmpeg avutil include directory"
)

find_path(SWSCALE_INCLUDE_DIR 
    NAMES libswscale/swscale.h
    PATHS ${FFMPEG_INSTALL_DIR}
    PATH_SUFFIXES include include/ffmpeg
    DOC "FFmpeg swscale include directory"
)

find_path(SWRESAMPLE_INCLUDE_DIR 
    NAMES libswresample/swresample.h
    PATHS ${FFMPEG_INSTALL_DIR}
    PATH_SUFFIXES include include/ffmpeg
    DOC "FFmpeg swresample include directory"
)

if(AVFORMAT_INCLUDE_DIR AND AVCODEC_INCLUDE_DIR AND AVUTIL_INCLUDE_DIR AND SWSCALE_INCLUDE_DIR AND SWRESAMPLE_INCLUDE_DIR)
    set(FFMPEG_INCLUDE_DIRS ${AVFORMAT_INCLUDE_DIR} ${AVCODEC_INCLUDE_DIR} ${AVUTIL_INCLUDE_DIR} ${SWSCALE_INCLUDE_DIR} ${SWRESAMPLE_INCLUDE_DIR})
    
    # Find library files
    find_library(AVFORMAT_LIBRARY 
        NAMES avformat
        PATHS ${FFMPEG_INSTALL_DIR}
        PATH_SUFFIXES lib lib64
        DOC "FFmpeg avformat library"
    )
    
    find_library(AVCODEC_LIBRARY 
        NAMES avcodec
        PATHS ${FFMPEG_INSTALL_DIR}
        PATH_SUFFIXES lib lib64
        DOC "FFmpeg avcodec library"
    )
    
    find_library(AVUTIL_LIBRARY 
        NAMES avutil
        PATHS ${FFMPEG_INSTALL_DIR}
        PATH_SUFFIXES lib lib64
        DOC "FFmpeg avutil library"
    )
    
    find_library(SWSCALE_LIBRARY 
        NAMES swscale
        PATHS ${FFMPEG_INSTALL_DIR}
        PATH_SUFFIXES lib lib64
        DOC "FFmpeg swscale library"
    )

    find_library(SWRESAMPLE_LIBRARY 
        NAMES swresample
        PATHS ${FFMPEG_INSTALL_DIR}
        PATH_SUFFIXES lib lib64
        DOC "FFmpeg swresample library"
    )
    
    set(FFMPEG_LIBRARIES ${AVFORMAT_LIBRARY} ${AVCODEC_LIBRARY} ${AVUTIL_LIBRARY} ${SWSCALE_LIBRARY} ${SWRESAMPLE_LIBRARY})
    message(STATUS "Found FFmpeg includes: ${FFMPEG_INCLUDE_DIRS}")
    message(STATUS "Found FFmpeg libraries: ${FFMPEG_LIBRARIES}")
else()
    message(FATAL_ERROR "FFmpeg not found at ${FFMPEG_INSTALL_DIR}. Please check FFMPEG_INSTALL_DIR variable.")
endif()

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/core/VideoDecoder.cpp
    src/core/VideoRenderer.cpp
    src/core/ConfigManager.cpp
    src/core/ConfigBridge.cpp
    src/core/AVDemuxer.cpp
    src/core/AudioDecoder.cpp
    src/core/AudioOutput.cpp
    src/core/VideoRenderer.h
    src/core/VideoDecoder.h
    src/core/AVDemuxer.h
    src/core/AudioDecoder.h
    src/core/AudioOutput.h
    resources.qrc
)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FFMPEG_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Quick
    Qt6::OpenGL
    Qt6::Qml
    Qt6::QuickControls2
    Qt6::Multimedia
    ${FFMPEG_LIBRARIES}
)