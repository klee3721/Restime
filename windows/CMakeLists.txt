cmake_minimum_required(VERSION 3.24)
project(RestimeWindows LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /permissive- /utf-8 /EHsc)
  add_compile_definitions(UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

add_library(restime_core
  src/codex_auth_reader.cpp
  src/codex_usage_reader.cpp
  src/session_usage_reader.cpp
  src/settings.cpp
  src/simple_json.cpp
  src/usage.cpp
  src/usage_formatter.cpp
)

target_include_directories(restime_core PUBLIC src)
target_link_libraries(restime_core PUBLIC winhttp advapi32)

add_executable(Restime WIN32
  src/app.cpp
  src/main.cpp
  src/refresh_coordinator.cpp
  src/tray_controller.cpp
)

target_link_libraries(Restime PRIVATE restime_core user32 shell32 gdi32 advapi32)

add_executable(restime_tests
  tests/test_main.cpp
)

target_link_libraries(restime_tests PRIVATE restime_core)
enable_testing()
add_test(NAME restime_tests COMMAND restime_tests)

if(MSVC)
  set_property(TARGET Restime restime_tests restime_core PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  target_compile_options(Restime PRIVATE $<$<CONFIG:Release>:/O2 /GL /Gy /Gw>)
  target_link_options(Restime PRIVATE $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>)
endif()
