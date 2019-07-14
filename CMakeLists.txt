CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)

ADD_COMPILE_OPTIONS($<$<CXX_COMPILER_ID:MSVC>:/MP> $<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:RELEASE>:/O2>> $<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:RELWITHDEBINFO>:/O2>>)

SET(PROJ_NAME installer_ncc)

PROJECT(${PROJ_NAME})

SET(DEPENDENCIES_DIR CACHE PATH "")

# hint to find qt in dependencies path
LIST(APPEND CMAKE_PREFIX_PATH ${QT_ROOT}/lib/cmake)

ADD_SUBDIRECTORY(src)
#ADD_SUBDIRECTORY(boss_dummy) Not sure what this accomplishes aside from making NCC lighter but it might be worth implementing this for 32bit and LOOT