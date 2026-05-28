# LuaEngine / Eluna integration for TurtleWoW.
#
# This CMakeLists.txt collects all Eluna source files into ELUNA_SRCS
# which is appended to the game library when BUILD_ELUNA=ON.
#
# The Eluna source tree is expected to be placed directly in this directory.
# Clone https://github.com/ElunaLuaEngine/Eluna into src/game/LuaEngine/
# so that files like LuaEngine.h, LuaEngine.cpp, etc. live here.
#
# Directory layout after cloning:
#   src/game/LuaEngine/
#     LuaEngine.h / .cpp
#     ElunaConfig.h / .cpp
#     ElunaCompat.h / .cpp
#     ElunaCreatureAI.h
#     ElunaEventMgr.h / .cpp
#     ElunaIncludes.h
#     ElunaInstanceAI.h / .cpp
#     ElunaLoader.h / .cpp
#     ElunaMgr.h / .cpp
#     ElunaSpellWrapper.h / .cpp
#     ElunaTemplate.h / .cpp
#     ElunaUtility.h / .cpp
#     LuaValue.h / .cpp
#     BindingMap.h
#     lmarshal.h / .cpp
#     hooks/*.cpp, hooks/*.h
#     methods/Methods.cpp
#     methods/CMangos/*.h    (CMaNGOS-specific method bindings)

# Collect all .cpp and .h files from the Eluna source tree
file(GLOB ELUNA_ROOT_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

file(GLOB ELUNA_HOOK_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/hooks/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/hooks/*.h"
)

file(GLOB ELUNA_METHOD_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/methods/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/methods/*.h"
)

file(GLOB ELUNA_CMANGOS_METHOD_HDRS
    "${CMAKE_CURRENT_SOURCE_DIR}/methods/CMangos/*.h"
)

# Exclude this CMakeLists.txt from source lists (file() may pick it up)
list(FILTER ELUNA_ROOT_SRCS EXCLUDE REGEX "CMakeLists\\.txt$")

set(ELUNA_SRCS
    ${ELUNA_ROOT_SRCS}
    ${ELUNA_HOOK_SRCS}
    ${ELUNA_METHOD_SRCS}
    ${ELUNA_CMANGOS_METHOD_HDRS}
    PARENT_SCOPE
)
