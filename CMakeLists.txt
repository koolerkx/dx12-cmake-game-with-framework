cmake_minimum_required(VERSION 4.0)

project("dx12-game-with-engine")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# TODO: Use msvc, the current approach take the llvm gcc clang as default
find_program(CLANG_TIDY_EXE NAMES clang-tidy)

if(CLANG_TIDY_EXE)
  message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
  set(CMAKE_CXX_CLANG_TIDY
    "${CLANG_TIDY_EXE}"
    "-p=${CMAKE_BINARY_DIR}"
    "-extra-arg=/EHsc")
else()
  message(WARNING "clang-tidy not found")
endif()

if(MSVC)
  add_compile_options(/wd4324)

  if(CMAKE_GENERATOR MATCHES "Visual Studio")
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<CONFIG:Debug,RelWithDebInfo>,EditAndContinue,ProgramDatabase>")
  else()
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<CONFIG:Debug,RelWithDebInfo>,Embedded,ProgramDatabase>")
  endif()
endif()

function(set_msvc_runtime target)
  if(MSVC)
    set_property(TARGET ${target} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    target_compile_options(${target} PRIVATE
      $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
      $<$<CXX_COMPILER_ID:MSVC>:/W4>
      $<$<CXX_COMPILER_ID:MSVC>:/MP>
      $<$<CXX_COMPILER_ID:MSVC>:/EHsc>
    )

    target_compile_definitions(${target} PRIVATE UNICODE _UNICODE)

    target_compile_definitions(${target} PRIVATE
      $<$<CONFIG:Debug>:DEBUG>
      $<$<CONFIG:Debug>:_DEBUG>
    )

  endif()
endfunction()

# サブプロジェクトを含めます。
add_subdirectory("app")
