include(FetchContent)

set(CUSTOM_ONNXRUNTIME_URL ""
    CACHE STRING "URL of a downloaded ONNX Runtime tarball")

set(CUSTOM_ONNXRUNTIME_HASH ""
    CACHE STRING "Hash of a downloaded ONNX Runtime tarball")

set(Onnxruntime_VERSION "1.23.2")

set(Onnxruntime_BASEURL "https://github.com/microsoft/onnxruntime/releases/download/v${Onnxruntime_VERSION}")

if(APPLE)
  set(Onnxruntime_URL "${Onnxruntime_BASEURL}/onnxruntime-osx-universal2-${Onnxruntime_VERSION}.tgz")
  set(Onnxruntime_HASH SHA256=49ae8e3a66ccb18d98ad3fe7f5906b6d7887df8a5edd40f49eb2b14e20885809)
elseif(MSVC)
  set(Onnxruntime_URL "${Onnxruntime_BASEURL}/onnxruntime-win-x64-${Onnxruntime_VERSION}.zip")
  set(OOnnxruntime_HASH SHA256=0b38df9af21834e41e73d602d90db5cb06dbd1ca618948b8f1d66d607ac9f3cd)
else()
  if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set(Onnxruntime_URL "${Onnxruntime_BASEURL}/onnxruntime-linux-aarch64-${Onnxruntime_VERSION}.tgz")
    set(Onnxruntime_HASH SHA256=70B6F536BB7AB5961D128E9DBD192368AC1513BFFB74FE92F97AAC342FBD0AC1)
  else()
    set(Onnxruntime_URL "${Onnxruntime_BASEURL}/onnxruntime-linux-x64-${Onnxruntime_VERSION}.tgz")
    set(Onnxruntime_HASH SHA256=1fa4dcaef22f6f7d5cd81b28c2800414350c10116f5fdd46a2160082551c5f9b) # 1.23.2
    # For GPU :
    # set(Onnxruntime_URL "${Onnxruntime_BASEURL}/onnxruntime-linux-x64-gpu-${Onnxruntime_VERSION}.tgz")
    # set(Onnxruntime_HASH SHA256=2083e361072a79ce16a90dcd5f5cb3ab92574a82a3ce0ac01e5cfa3158176f53) # 1.23.2
  endif()
endif()

set(FETCHCONTENT_QUIET FALSE)
FetchContent_Declare(
  onnxruntime
  URL ${Onnxruntime_URL}
  URL_HASH ${Onnxruntime_HASH}
  DOWNLOAD_EXTRACT_TIMESTAMP OLD)
FetchContent_MakeAvailable(onnxruntime)

if(APPLE)
  set(Onnxruntime_LIB "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.${Onnxruntime_VERSION}.dylib")
  target_link_libraries(${PROJECT_NAME} PRIVATE "${Onnxruntime_LIB}")
  target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC "${onnxruntime_SOURCE_DIR}/include")
  target_sources(${PROJECT_NAME} PRIVATE "${Onnxruntime_LIB}")
  set_property(SOURCE "${Onnxruntime_LIB}" PROPERTY MACOSX_PACKAGE_LOCATION Frameworks)
  source_group("Frameworks" FILES "${Onnxruntime_LIB}")
  add_custom_command(
    TARGET "${PROJECT_NAME}"
    POST_BUILD
    COMMAND
      ${CMAKE_INSTALL_NAME_TOOL} -change "@rpath/libonnxruntime.${Onnxruntime_VERSION}.dylib"
      "@loader_path/../Frameworks/libonnxruntime.${Onnxruntime_VERSION}.dylib" $<TARGET_FILE:${PROJECT_NAME}>)
elseif(MSVC)
  add_library(Ort INTERFACE)
  target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC "${onnxruntime_SOURCE_DIR}/include")

  target_link_libraries(${PROJECT_NAME} PRIVATE "${onnxruntime_SOURCE_DIR}/lib/onnxruntime.lib")
  target_link_libraries(${PROJECT_NAME} PRIVATE "${onnxruntime_SOURCE_DIR}/lib/onnxruntime_providers_shared.lib")

#  set(Onnxruntime_LIB_NAMES
#      session;providers_shared;providers_dml;optimizer;providers;framework;graph;util;mlas;common;flatbuffers)
#  set(Onnxruntime_LIB_NAMES
#          onnxruntime;onnxruntime_providers_shared)
#  foreach(lib_name IN LISTS Onnxruntime_LIB_NAMES)
#    add_library(Ort::${lib_name} STATIC IMPORTED)
#    set_target_properties(Ort::${lib_name} PROPERTIES IMPORTED_LOCATION
#                                                      ${onnxruntime_SOURCE_DIR}/lib/onnxruntime_${lib_name}.lib)
#    set_target_properties(Ort::${lib_name} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${onnxruntime_SOURCE_DIR}/include)
#    target_link_libraries(Ort INTERFACE Ort::${lib_name})
#  endforeach()

#  set(Onnxruntime_EXTERNAL_LIB_NAMES
#          onnx;onnx_proto;libprotobuf-lite;re2;absl_throw_delegate;absl_hash;absl_city;absl_low_level_hash;absl_raw_hash_set
#  )
  set(Onnxruntime_EXTERNAL_LIB_NAMES
          onnxruntime;onnxruntime_providers_shared
  )
#  foreach(lib_name IN LISTS Onnxruntime_EXTERNAL_LIB_NAMES)
#    add_library(Ort::${lib_name} STATIC IMPORTED)
#    set_target_properties(Ort::${lib_name} PROPERTIES IMPORTED_LOCATION ${onnxruntime_SOURCE_DIR}/lib/${lib_name}.lib)
#    target_link_libraries(Ort INTERFACE Ort::${lib_name})
#  endforeach()

#  add_library(Ort::DirectML SHARED IMPORTED)
#  set_target_properties(Ort::DirectML PROPERTIES IMPORTED_LOCATION ${onnxruntime_SOURCE_DIR}/bin/DirectML.dll)
#  set_target_properties(Ort::DirectML PROPERTIES IMPORTED_IMPLIB ${onnxruntime_SOURCE_DIR}/bin/DirectML.lib)
#
#  target_link_libraries(Ort INTERFACE Ort::DirectML d3d12.lib dxgi.lib dxguid.lib Dxcore.lib)

#  target_link_libraries(${PROJECT_NAME} PRIVATE Ort)

#  install(IMPORTED_RUNTIME_ARTIFACTS Ort::DirectML DESTINATION "obs-plugins/64bit")
else()
  if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set(Onnxruntime_LINK_LIBS "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.so.${Onnxruntime_VERSION}")
    set(Onnxruntime_INSTALL_LIBS ${Onnxruntime_LINK_LIBS})
  else()
    set(Onnxruntime_LINK_LIBS "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.so.${Onnxruntime_VERSION}")
    # For GPU :
    # set(Onnxruntime_LINK_LIBS
    #         "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.so.${Onnxruntime_VERSION}"
    #         "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime_providers_shared.so"
    #         "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime_providers_cuda.so"
    #         "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime_providers_tensorrt.so"
    # )
    set(Onnxruntime_INSTALL_LIBS
        ${Onnxruntime_LINK_LIBS}
        #"${onnxruntime_SOURCE_DIR}/lib/libonnxruntime_providers_shared.so"
        #"${onnxruntime_SOURCE_DIR}/lib/libonnxruntime_providers_cuda.so"
        #"${onnxruntime_SOURCE_DIR}/lib/libonnxruntime_providers_tensorrt.so"
    )
  endif()
  target_link_libraries(${PROJECT_NAME} PRIVATE ${Onnxruntime_LINK_LIBS})
  target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC "${onnxruntime_SOURCE_DIR}/include")
  install(FILES ${Onnxruntime_INSTALL_LIBS} DESTINATION "${CMAKE_INSTALL_LIBDIR}/obs-plugins/${PROJECT_NAME}")
  set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN/${PROJECT_NAME}")
endif()
