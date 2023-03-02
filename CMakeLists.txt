cmake_minimum_required(VERSION 3.15)
project(sdp VERSION 1.0)

# checks if set up rpath exists for install
if(COMMAND set_up_rpath)
    set_up_rpath()
else()
    message("Set up rpath not defined!")
endif()


set(LIB_SOURCES
      src/sdp.c
      src/sdp_base64.c
)

add_library(${PROJECT_NAME} ${LIB_SOURCES})

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
set_target_properties(sdp PROPERTIES
                    POSITION_INDEPENDENT_CODE ${BUILD_SHARED_LIBS}
)

target_include_directories(${PROJECT_NAME} PUBLIC
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                            $<INSTALL_INTERFACE:include>
)

target_compile_definitions(${PROJECT_NAME} PRIVATE "SDP_API_EXPORTS")
target_compile_options(${PROJECT_NAME} PRIVATE "-fvisibility=hidden")
target_compile_options(${PROJECT_NAME} PRIVATE "-std=gnu99")

target_link_libraries(${PROJECT_NAME} PRIVATE futils ulog)

install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}-targets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        )
