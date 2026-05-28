# Build Lua from source via FetchContent.
# Based on Niam5/Eluna-CMaNGOS-Classic dep/lualib/lua/CMakeLists.txt.
#
# Supports Lua 5.1, 5.2, 5.3, 5.4. Default for Eluna-CMaNGOS is 5.2.

cmake_minimum_required(VERSION 3.11)
project(lualib C)

include(FetchContent)

# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24+
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()

# ---------- Source archives for each Lua version ----------
FetchContent_Declare(lua51
  URL "https://www.lua.org/ftp/lua-5.1.5.tar.gz"
  URL_HASH SHA256=2640fc56a795f29d28ef15e13c34a47e223960b0240e8cb0a82d9b0738695333
)
FetchContent_Declare(lua52
  URL "https://www.lua.org/ftp/lua-5.2.4.tar.gz"
  URL_HASH SHA256=b9e2e4aad6789b3b63a056d442f7b39f0ecfca3ae0f1fc0ae4e9614401b69f4b
)
FetchContent_Declare(lua53
  URL "https://www.lua.org/ftp/lua-5.3.6.tar.gz"
  URL_HASH SHA256=fc5fd69bb8736323f026672b1b7235da613d7177e72558893a0bdcd320466d60
)
FetchContent_Declare(lua54
  URL "https://www.lua.org/ftp/lua-5.4.4.tar.gz"
  URL_HASH SHA256=164c7849653b80ae67bec4b7473b884bf5cc8d2dca05571ef966587c8990f4b7
)

FetchContent_MakeAvailable(${LUA_VERSION})

# Find the fetched source directory
set(LUA_SRC_DIR "${${LUA_VERSION}_SOURCE_DIR}/src")

# Suppress compiler warnings in third-party Lua code
if(MSVC)
  add_compile_options(/W0)
else()
  add_compile_options(-w)
endif()

# Collect all .c files from the Lua source, excluding lua.c and luac.c
# (those are the standalone interpreter/compiler entry points)
file(GLOB LUA_SOURCES "${LUA_SRC_DIR}/*.c")
list(FILTER LUA_SOURCES EXCLUDE REGEX "lua\\.c$")
list(FILTER LUA_SOURCES EXCLUDE REGEX "luac\\.c$")

# Collect headers
file(GLOB LUA_HEADERS "${LUA_SRC_DIR}/*.h")

# Build as static or shared library
if(LUA_STATIC)
  add_library(lualib STATIC ${LUA_SOURCES} ${LUA_HEADERS})
else()
  add_library(lualib SHARED ${LUA_SOURCES} ${LUA_HEADERS})
endif()

# Make Lua headers available to consumers via lualib's public interface
target_include_directories(lualib PUBLIC ${LUA_SRC_DIR})

# Platform-specific definitions
if(WIN32)
  # Avoid DLL import/export complexity when static
  if(NOT LUA_STATIC)
    target_compile_definitions(lualib PRIVATE LUA_BUILD_AS_DLL)
  endif()
elseif(APPLE)
  target_compile_definitions(lualib PRIVATE LUA_USE_MACOSX)
elseif(UNIX)
  target_compile_definitions(lualib PRIVATE LUA_USE_LINUX)
  target_link_libraries(lualib PRIVATE dl)
endif()

# Put in Dependencies folder in IDEs
set_target_properties(lualib PROPERTIES FOLDER "Dependencies")

# Install
install(TARGETS lualib
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
install(FILES ${LUA_HEADERS} DESTINATION include/lua)
