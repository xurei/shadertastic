cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

if(DEV_MODE)
  message("SHADERTASTIC: DEV MODE ENABLED")
  add_compile_definitions(DEV_MODE)
  if(DEV_LOG_PATH)
    set(DEV_LOG_PATH "" CACHE STRING "Value of DEV_LOG_PATH")
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEV_LOG_PATH="${DEV_LOG_PATH}")
  endif()
endif()

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h)
file(GLOB sources_CPP
  "src/*.cpp"
  "src/**/*.cpp"
)
file(GLOB sources_HPP
  "src/*.hpp"
  "src/**/*.hpp"
)
file(GLOB onnxmediapipe_CPP
  "onnxmediapipe/src/*"
  "onnxmediapipe/src/filtering/*"
)

target_include_directories(${PROJECT_NAME} PRIVATE onnxmediapipe/include)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
  src/version.h
  ${sources_CPP}
  ${onnxmediapipe_CPP}
)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

# ONNXRuntime & OpenCV dependencies
# Disable GPU (for now)
target_compile_definitions(${PROJECT_NAME} PRIVATE DISABLE_ONNXRUNTIME_GPU)
message("Fetching ONNX Runtime...")
include(cmake/FetchOnnxruntime.cmake)
message("Fetching OpenCV...")
include(cmake/FetchOpenCV.cmake)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenCV)

# Using DELAYLOAD to defer onnxruntime loading to a different path
if(MSVC)
  target_link_options(${PROJECT_NAME} PRIVATE "/DELAYLOAD:onnxruntime.dll")

  target_link_libraries(${PROJECT_NAME} PRIVATE delayimp)
endif ()

if(OS_WINDOWS)
  #    message("Windows build")
elseif(OS_MACOS)
  # TODO
elseif(OS_LINUX)
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND patchelf --replace-needed libonnxruntime.so.1 libonnxruntime.so.${Onnxruntime_VERSION} $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Fixing ONNX dependency name for portability"
  )

  # Packaging - force the dependencies so Qt is not taken with it; OBS will have it anyways
endif()
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF CACHE BOOL "" FORCE)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.38), libgcc-s1 (>= 4.0), libqt6core6t64 (>= 6.4.0), libstdc++6 (>= 13.1), obs-studio, zlib1g (>= 1:1.1.4)" CACHE STRING "" FORCE)
set(CPACK_DEBIAN_shadertastic_PACKAGE_DEPENDS "libc6 (>= 2.38), libgcc-s1 (>= 4.0), libqt6core6t64 (>= 6.4.0), libstdc++6 (>= 13.1), obs-studio, zlib1g (>= 1:1.1.4)" CACHE STRING "" FORCE)

# libzip
# Disable warnings for libzip
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libs" FORCE)
set(BUILD_DOC OFF CACHE BOOL "Build documentation" FORCE)
set(BUILD_REGRESS OFF CACHE BOOL "Build regression tests" FORCE)
set(ENABLE_COMMONCRYPTO OFF CACHE BOOL "" FORCE)
set(ENABLE_GNUTLS OFF CACHE BOOL "" FORCE)
set(ENABLE_MBEDTLS OFF CACHE BOOL "" FORCE)
set(ENABLE_OPENSSL OFF CACHE BOOL "" FORCE)
set(ENABLE_BZIP2 OFF CACHE BOOL "" FORCE)
set(ENABLE_LZMA OFF CACHE BOOL "" FORCE)
set(ENABLE_ZSTD OFF CACHE BOOL "" FORCE)
set(LIBZIP_DO_INSTALL OFF CACHE BOOL "" FORCE)
set(ENABLE_ZLIB ON CACHE BOOL "" FORCE)
add_subdirectory(vendor/libzip)
target_link_libraries(${PROJECT_NAME} PRIVATE zip)
target_compile_options(zip PRIVATE -w)

# Désactive /WX uniquement pour libzip
if(MSVC)
  target_compile_options(zip PRIVATE /wd4098 /wd4273)  # warnings fréquents pour libzip
  set_target_properties(zip PROPERTIES LINK_FLAGS "/ignore:4098")
endif()

# zlib
find_package(ZLIB REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE ${ZLIB_LIBRARIES})
include_directories(${ZLIB_INCLUDE_DIRS})

# Make sure CMake does not try to compile hlsl files in the data folder
file(GLOB_RECURSE shaders_HLSL
  "*.hlsl"
  "**/*.hlsl"
)
set_source_files_properties(SOURCE ${shaders_HLSL} PROPERTIES VS_SETTINGS "ExcludedFromBuild=true")

# Activate std:c++20
if(MSVC)
  add_definitions(/std:c++20)
endif()

# Installer
configure_file("installer.iss.in"
  "${PROJECT_BINARY_DIR}/installer.iss"
)