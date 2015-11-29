CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

PROJECT(BOSSDummy)

CMAKE_POLICY(SET CMP0020 NEW)
CMAKE_POLICY(SET CMP0043 NEW)

SET(PROJ_ARCH x86 CACHE STRING "build architecture, has to be x86 or x64")

FILE(GLOB ${PROJ_NAME}_SRCS *.cpp)
FILE(GLOB ${PROJ_NAME}_HDRS *.h)

SET(default_project_path "${CMAKE_SOURCE_DIR}/..")
GET_FILENAME_COMPONENT(${default_project_path} ${default_project_path} REALPATH)

SET(project_path "${default_project_path}" CACHE PATH "path to the other mo projects")
#SET(lib_path "${project_path}/../../install/libs")

INCLUDE_DIRECTORIES(${project_path}/uibase/src)
#LINK_DIRECTORIES(${lib_path})

set(libname boss_${PROJ_ARCH})

ADD_LIBRARY(${libname} SHARED ${${PROJ_NAME}_HDRS} ${${PROJ_NAME}_SRCS})

SET_TARGET_PROPERTIES(${libname} PROPERTIES COMPILE_FLAGS /GL)
SET_TARGET_PROPERTIES(${libname} PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/LTCG /LARGEADDRESSAWARE /OPT:REF /OPT:ICF")

###############
## Installation

INSTALL(TARGETS ${libname}
        RUNTIME DESTINATION NCC/data)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${libname}.pdb DESTINATION pdb)
