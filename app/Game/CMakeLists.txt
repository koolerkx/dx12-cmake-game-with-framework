add_library(game STATIC
    game.cpp
    game.h

    game_object.h
    game_object.cpp
    Component/component.h
    Component/transform_component.h
    Component/transform_component.cpp
    Component/renderer_component.h
    Component/renderer_component.cpp

    Scene/scene.h
    Scene/scene.cpp
)

set_msvc_runtime(game)

target_include_directories(game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(game PUBLIC graphic)
