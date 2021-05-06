cmake_minimum_required(VERSION 3.0.0)
add_definitions(-DUNICODE -D_UNICODE)
project(Blank VERSION 0.1.0)

include(CTest)
enable_testing()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast")
set(CMAKE_C_FLAGS /source-charset:utf-8)

add_executable(${PROJECT_NAME} WIN32 
    main.cpp
    ./Common/lodepng.cpp
    ./imgui/imgui.cpp
    ./imgui/imgui_draw.cpp
    ./imgui/imgui_tables.cpp
    ./imgui/imgui_widgets.cpp
    ./imgui/imgui_impl_dx12.cpp
    ./imgui/imgui_impl_win32.cpp
    ./imgui/imgui_demo.cpp 
    )

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/DirectXMath/Inc
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        d3d12.lib
        dxgi.lib
        dxcompiler.lib
)


if(WIN32)
  add_custom_command(TARGET ${PROJECT_NAME}
                     PRE_BUILD
                     COMMAND ./compileshader_debug.bat
                     WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                    )

endif()

set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
