# Lua library wrapper for Eluna integration.
# Selects between Lua versions; defaults to Lua 5.2 for CMaNGOS Classic Eluna.
#
# Follows the same pattern as Niam5/Eluna-CMaNGOS-Classic dep/lualib.

set(LUA_VERSION "lua52" CACHE STRING "Lua version to use")
set_property(CACHE LUA_VERSION PROPERTY STRINGS lua51 lua52 lua53 lua54)
message(STATUS "Lua version: ${LUA_VERSION}")

if(WIN32)
  option(LUA_STATIC "Link Lua statically" ON)
else()
  option(LUA_STATIC "Link Lua statically" OFF)
endif()

if(LUA_STATIC)
  message(STATUS "Lua linking: static")
else()
  message(STATUS "Lua linking: dynamic")
endif()

add_subdirectory(lua)
