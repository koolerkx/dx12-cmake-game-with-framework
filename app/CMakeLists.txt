add_executable(app WIN32
    main.cpp
    Application/Application.h
    Application/Application.cpp
    Application/timer_updater.h
    Application/timer_updater.cpp
)

set_msvc_runtime(app)

if(WIN32)
    target_compile_definitions(app PRIVATE _WIN32_WINNT=0x0A00)
endif()

target_link_libraries(app PRIVATE
    d3d12.lib
    dxgi.lib
    d3dcompiler.lib
    uuid.lib
)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(Graphic)

target_link_libraries(app PRIVATE graphic)
