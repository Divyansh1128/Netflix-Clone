cmake_minimum_required(VERSION 3.20)
if(CMAKE_HOST_APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum supported macOS version")
endif()
project(CaptureOverlay VERSION 1.0.0 LANGUAGES CXX)

if(WIN32)
    enable_language(RC)
    add_executable(CaptureOverlay WIN32 src/main.cpp src/windows.rc)
    target_include_directories(CaptureOverlay PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_compile_features(CaptureOverlay PRIVATE cxx_std_17)
    target_compile_definitions(CaptureOverlay PRIVATE UNICODE _UNICODE NOMINMAX
        WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
    target_link_libraries(CaptureOverlay PRIVATE user32 gdi32 comctl32)
    if(MSVC)
        target_compile_options(CaptureOverlay PRIVATE /W4 /permissive- /utf-8)
        # Embed our compatibility/DPI manifest via the resource, not a second manifest.
        target_link_options(CaptureOverlay PRIVATE /MANIFEST:NO)
        # Statically link the runtime: the built executable needs no VC runtime installer.
        set_property(TARGET CaptureOverlay PROPERTY MSVC_RUNTIME_LIBRARY
            "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    elseif(MINGW)
        target_link_options(CaptureOverlay PRIVATE -municode -static-libgcc -static-libstdc++)
    endif()
elseif(APPLE)
    enable_language(OBJCXX)
    add_executable(CaptureOverlay MACOSX_BUNDLE src/main_mac.mm)
    target_compile_features(CaptureOverlay PRIVATE cxx_std_17)
    target_compile_options(CaptureOverlay PRIVATE -fobjc-arc -Wall -Wextra -Wpedantic)
    target_link_libraries(CaptureOverlay PRIVATE "-framework Cocoa")
    set_target_properties(CaptureOverlay PROPERTIES
        OBJCXX_STANDARD 17
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist")
else()
    message(FATAL_ERROR "CaptureOverlay supports native Windows and macOS builds only.")
endif()
