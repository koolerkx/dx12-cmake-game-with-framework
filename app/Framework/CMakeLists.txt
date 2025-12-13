add_library(framework STATIC
    Logging/logger.h
    Logging/logger.cpp
    Logging/sinks.h
    Logging/debug_sink.cpp
    Logging/console_sink.cpp

    Error/framework_bootstrap_log.h
    Error/framework_bootstrap_log.cpp
    Error/framework_error.h
    Error/framework_error.cpp

    utils.h
    types.h
)

set_msvc_runtime(framework)

target_include_directories(framework PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
