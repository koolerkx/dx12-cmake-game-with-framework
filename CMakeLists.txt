cmake_minimum_required(VERSION 3.15)

project("dx12-game-with-engine")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
  if(CMAKE_GENERATOR MATCHES "Visual Studio")
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<CONFIG:Debug,RelWithDebInfo>,EditAndContinue,ProgramDatabase>")
  else()
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<CONFIG:Debug,RelWithDebInfo>,Embedded,ProgramDatabase>")
  endif()
endif()

# サブプロジェクトを含めます。
add_subdirectory("App")
