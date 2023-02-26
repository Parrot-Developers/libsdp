cmake_minimum_required(VERSION 3.15)
project(libsdp VERSION 1.0)

set(LIB_SOURCES
      src/sdp.c
      src/sdp_base64.c
)

add_library(sdp ${LIB_SOURCES})

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
set_target_properties(sdp PROPERTIES
                    POSITION_INDEPENDENT_CODE ${BUILD_SHARED_LIBS}
)

target_include_directories(sdp PUBLIC
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                            $<INSTALL_INTERFACE:include>
)

target_compile_definitions(sdp PRIVATE "SDP_API_EXPORTS")
target_compile_options(sdp PRIVATE "-fvisibility=hidden")
target_compile_options(sdp PRIVATE "-std=gnu99")

target_link_libraries(sdp futils ulog)