project(vfs.afp)

cmake_minimum_required(VERSION 2.6)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(AfpClient REQUIRED)
find_package(kodi REQUIRED)
find_package(kodiplatform REQUIRED)

include_directories(${KODI_INCLUDE_DIR}
                    ${KODIPLATFORM_INCLUDE_DIRS}
                    ${AFPCLIENT_INCLUDE_DIRS})

set(AFP_SOURCES src/AFPConnection.cpp
                src/AFPFile.cpp)

set(DEPLIBS ${AFPCLIENT_LIBRARIES})

add_definitions(-D_LARGEFILE64_SOURCE
                -D_FILE_OFFSET_BITS=64
                -D__STDC_FORMAT_MACROS)

build_addon(vfs.afp AFP DEPLIBS)
