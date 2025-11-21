set(SHADER_OUT ${CMAKE_SOURCE_DIR}/Content/shaders)

find_program(DXC_EXECUTABLE
    NAMES dxc dxc.exe
    PATHS
    "$ENV{WindowsSdkDir}bin/$ENV{WindowsSDKVersion}x64"
    "$ENV{WindowsSdkDir}bin/x64"
    "C:/Program Files (x86)/Windows Kits/10/bin/*/x64"
    "$ENV{VCToolsInstallDir}bin/Hostx64/x64"
    DOC "DirectX Shader Compiler (dxc.exe)"
)

if(NOT DXC_EXECUTABLE AND CMAKE_GENERATOR MATCHES "Ninja")
    message(FATAL_ERROR "DirectX Shader Compiler (dxc.exe) not found for Ninja generator. Please install Windows SDK 10.")
endif()

if(DXC_EXECUTABLE)
    message(STATUS "Found DXC: ${DXC_EXECUTABLE}")
endif()

function(target_add_hlsl TARGET_NAME MODEL SHADER_TYPE)
    set(FILES ${ARGN})

    target_sources(${TARGET_NAME} PRIVATE ${FILES})

    # Convert shader model format ("5.0" -> "5_0", "6.5" -> "6_5")
    string(REPLACE "." "_" MODEL_UNDERSCORE ${MODEL})

    # Map SHADER_TYPE to DXC profile prefix
    if(SHADER_TYPE STREQUAL "Vertex" OR SHADER_TYPE STREQUAL "vs")
        set(DXC_PROFILE "vs")
    elseif(SHADER_TYPE STREQUAL "Pixel" OR SHADER_TYPE STREQUAL "ps")
        set(DXC_PROFILE "ps")
    elseif(SHADER_TYPE STREQUAL "Compute" OR SHADER_TYPE STREQUAL "cs")
        set(DXC_PROFILE "cs")
    elseif(SHADER_TYPE STREQUAL "Geometry" OR SHADER_TYPE STREQUAL "gs")
        set(DXC_PROFILE "gs")
    elseif(SHADER_TYPE STREQUAL "Hull" OR SHADER_TYPE STREQUAL "gs")
        set(DXC_PROFILE "hs")
    elseif(SHADER_TYPE STREQUAL "Domain" OR SHADER_TYPE STREQUAL "ds")
        set(DXC_PROFILE "ds")
    elseif(SHADER_TYPE STREQUAL "Mesh" OR SHADER_TYPE STREQUAL "ms")
        set(DXC_PROFILE "ms")
        if(${MODEL} VERSION_LESS "6.5")
            message(WARNING "Mesh Shader is added to target but using shader model lower than 6.5")
            set(SM_VERSION "6_5")
        endif()
    elseif(SHADER_TYPE STREQUAL "Amplification" OR SHADER_TYPE STREQUAL "as")
        set(DXC_PROFILE "as")
        if(${MODEL} VERSION_LESS "6.5")
            message(WARNING "Amplification Shader is added to target but using shader model lower than 6.5")
            set(SM_VERSION "6_5")
        endif()
    elseif(SHADER_TYPE STREQUAL "Library" OR SHADER_TYPE STREQUAL "lib")
        set(DXC_PROFILE "lib")
    else()
        message(FATAL_ERROR "Unknown shader type: ${SHADER_TYPE}")
    endif()

    foreach(HLSL_FILE ${FILES})
        get_filename_component(FILE_NAME_FULL ${HLSL_FILE} NAME)
        string(REPLACE ".hlsl" "" FILE_NAME_WE "${FILE_NAME_FULL}")
        get_filename_component(HLSL_FILE_ABS ${HLSL_FILE} ABSOLUTE)

        set(OUTPUT_FILE "${SHADER_OUT}/${FILE_NAME_WE}.cso")

        # Visual Studio generator - use built-in shader compilation
        if(MSVC AND NOT CMAKE_GENERATOR MATCHES "Ninja")
            set_source_files_properties(${HLSL_FILE} PROPERTIES
                VS_SHADER_TYPE ${SHADER_TYPE}
                VS_SHADER_MODEL ${MODEL}
                VS_SHADER_ENTRYPOINT "main"
                VS_SHADER_OBJECT_FILE_NAME "${OUTPUT_FILE}"
                VS_SHADER_DISABLE_OPTIMIZATIONS $<$<CONFIG:Debug>:true>
                VS_SHADER_ENABLE_DEBUG $<$<CONFIG:Debug>:true>
            )

            if(${MODEL} VERSION_GREATER_EQUAL "6.0")
                set_property(SOURCE ${HLSL_FILE} APPEND PROPERTY VS_SHADER_FLAGS "/all_resources_bound")
            endif()


        else() # Ninja generator - use DXC manually
            # Build DXC command
            set(DXC_ARGS
                /T ${DXC_PROFILE}_${MODEL_UNDERSCORE}
                /E main
                /Fo "${OUTPUT_FILE}"
            )

            # Debug flags
            if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                list(APPEND DXC_ARGS
                    /Zi # Enable debug information
                    /Od # Disable optimizations
                    /Qembed_debug # Embed debug info
                )
            else()
                list(APPEND DXC_ARGS /O3) # Maximum optimization
            endif()

            # Shader Model 6.0+ flags
            if(${MODEL} VERSION_GREATER_EQUAL "6.0")
                list(APPEND DXC_ARGS /all_resources_bound)
            endif()

            # Add custom command for Ninja
            add_custom_command(
                OUTPUT ${OUTPUT_FILE}
                COMMAND ${CMAKE_COMMAND} -E make_directory SHADER_OUT
                COMMAND ${DXC_EXECUTABLE} ${DXC_ARGS} "${HLSL_FILE_ABS}"
                MAIN_DEPENDENCY ${HLSL_FILE_ABS}
                COMMENT "Compiling HLSL ${SHADER_TYPE} Shader: ${HLSL_FILE}"
                VERBATIM
            )

            # Create a custom target for this shader
            set(SHADER_TARGET "${TARGET_NAME}_${FILE_NAME_WE}")
            add_custom_target(${SHADER_TARGET} DEPENDS ${OUTPUT_FILE})
            add_dependencies(${TARGET_NAME} ${SHADER_TARGET})

            # Mark shader file as header-only for IDE
            set_source_files_properties(${HLSL_FILE} PROPERTIES
                HEADER_FILE_ONLY TRUE
            )
        endif()
    endforeach()

    source_group("Shaders\\${SHADER_TYPE}" FILES ${FILES})
endfunction()

# .vs.hlsl → Vertex Shader
# .ps.hlsl → Pixel Shader
# .cs.hlsl → Compute Shader
# .gs.hlsl → Geometry Shader
# .hs.hlsl → Hull Shader
# .ds.hlsl → Domain Shader
# .ms.hlsl → Mesh Shader
# .as.hlsl → Amplification Shader
# .lib.hlsl → Shader Library (ray tracing)
function(_detect_shader_type SHADER_TYPE FILENAME)
    get_filename_component(FULL_EXT ${FILENAME} EXT)
    string(TOLOWER "${FULL_EXT}" FULL_EXT)
    string(REGEX MATCH "\\.([a-z0-9_]+)\\.hlsl$" MATCH_RESULT "${FULL_EXT}")

    # Extract the shader type from patterns like .vs.hlsl, .ps.hlsl, etc.
    if(MATCH_RESULT)
        set(TAG "${CMAKE_MATCH_1}")

        if(TAG STREQUAL "vs")
            set(${SHADER_TYPE} "Vertex" PARENT_SCOPE)
        elseif(TAG STREQUAL "ps")
            set(${SHADER_TYPE} "Pixel" PARENT_SCOPE)
        elseif(TAG STREQUAL "cs")
            set(${SHADER_TYPE} "Compute" PARENT_SCOPE)
        elseif(TAG STREQUAL "gs")
            set(${SHADER_TYPE} "Geometry" PARENT_SCOPE)
        elseif(TAG STREQUAL "hs")
            set(${SHADER_TYPE} "Hull" PARENT_SCOPE)
        elseif(TAG STREQUAL "ds")
            set(${SHADER_TYPE} "Domain" PARENT_SCOPE)
        elseif(TAG STREQUAL "ms")
            set(${SHADER_TYPE} "Mesh" PARENT_SCOPE)
        elseif(TAG STREQUAL "as")
            set(${SHADER_TYPE} "Amplification" PARENT_SCOPE)
        elseif(TAG STREQUAL "lib")
            set(${SHADER_TYPE} "Library" PARENT_SCOPE)
        else()
            set(${SHADER_TYPE} "" PARENT_SCOPE)
            message(WARNING "Unknown shader tag '${TAG}' in file: ${FILENAME}")
        endif()
    else()
        set(${SHADER_TYPE} "" PARENT_SCOPE)
        message(WARNING "Unknown shader tag '${TAG}' in file: ${FILENAME}")
    endif()
endfunction()

function(target_add_hlsl_auto TARGET_NAME MODEL)
    set(FILES ${ARGN})

    # Group files by shader type
    set(VERTEX_SHADERS "")
    set(PIXEL_SHADERS "")
    set(COMPUTE_SHADERS "")
    set(GEOMETRY_SHADERS "")
    set(HULL_SHADERS "")
    set(DOMAIN_SHADERS "")
    set(LIBRARY_SHADERS "")
    set(UNKNOWN_SHADERS "")

    foreach(FILE ${FILES})
        _detect_shader_type(SHADER_TYPE ${FILE})

        if(SHADER_TYPE STREQUAL "Vertex")
            list(APPEND VERTEX_SHADERS ${FILE})
        elseif(SHADER_TYPE STREQUAL "Pixel")
            list(APPEND PIXEL_SHADERS ${FILE})
        elseif(SHADER_TYPE STREQUAL "Compute")
            list(APPEND COMPUTE_SHADERS ${FILE})
        elseif(SHADER_TYPE STREQUAL "Geometry")
            list(APPEND GEOMETRY_SHADERS ${FILE})
        elseif(SHADER_TYPE STREQUAL "Hull")
            list(APPEND HULL_SHADERS ${FILE})
        elseif(SHADER_TYPE STREQUAL "Domain")
            list(APPEND DOMAIN_SHADERS ${FILE})
        elseif(SHADER_TYPE STREQUAL "Library")
            list(APPEND LIBRARY_SHADERS ${FILE})
        else()
            list(APPEND UNKNOWN_SHADERS ${FILE})
        endif()
    endforeach()

    # Report unknown shaders
    if(UNKNOWN_SHADERS)
        message(WARNING "Could not detect shader type for files: ${UNKNOWN_SHADERS}. Use naming convention like 'shader.vs.hlsl' or call target_add_hlsl directly.")
    endif()

    # Compile each group
    if(VERTEX_SHADERS)
        target_add_hlsl(${TARGET_NAME} ${MODEL} "Vertex" ${VERTEX_SHADERS})
    endif()

    if(PIXEL_SHADERS)
        target_add_hlsl(${TARGET_NAME} ${MODEL} "Pixel" ${PIXEL_SHADERS})
    endif()

    if(COMPUTE_SHADERS)
        target_add_hlsl(${TARGET_NAME} ${MODEL} "Compute" ${COMPUTE_SHADERS})
    endif()

    if(GEOMETRY_SHADERS)
        target_add_hlsl(${TARGET_NAME} ${MODEL} "Geometry" ${GEOMETRY_SHADERS})
    endif()

    if(HULL_SHADERS)
        target_add_hlsl(${TARGET_NAME} ${MODEL} "Hull" ${HULL_SHADERS})
    endif()

    if(DOMAIN_SHADERS)
        target_add_hlsl(${TARGET_NAME} ${MODEL} "Domain" ${DOMAIN_SHADERS})
    endif()

    if(LIBRARY_SHADERS)
        target_add_hlsl(${TARGET_NAME} ${MODEL} "Library" ${LIBRARY_SHADERS})
    endif()
endfunction()
