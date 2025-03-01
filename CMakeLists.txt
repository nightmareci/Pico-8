# Do not modify this file unless absolutely necessary.
# All project-specific settings can be found in the file:
# project_config.cmake

cmake_minimum_required(VERSION 3.10)

option(BUILD_FOR_NOKIA_NGAGE "Build for Nokia N-Gage" OFF)
option (BUILD_CI_WORKFLOW "Build CI workflow" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/project_config.cmake)

if (BUILD_FOR_NOKIA_NGAGE)

  project(game C CXX)

  add_compile_definitions(__NGAGE__)

  set(UID1 0x1000007a) # KExecutableImageUidValue, e32uid.h
  set(UID2 0x100039ce) # KAppUidValue16, apadef.h

  set(game_static_libs
    ${EPOC_EXTRAS}/lib/armi/urel/SDL3.lib
    ${EPOC_EXTRAS}/lib/armi/urel/SDL3_mixer.lib)

  set(game_libs
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/armi/urel/z8lua.lib
    ${EPOC_LIB}/NRenderer.lib
    ${EPOC_LIB}/3dtypes.a
    ${EPOC_LIB}/cone.lib
    ${EPOC_PLATFORM}/gcc/lib/gcc-lib/arm-epoc-pe/2.9-psion-98r2/libgcc.a
    ${EPOC_PLATFORM}/ngagesdk/lib/gcc/arm-epoc-pe/4.6.4/libgcc_ngage.a
    ${EPOC_LIB}/mediaclientaudiostream.lib
    ${EPOC_LIB}/charconv.lib
    ${EPOC_LIB}/bitgdi.lib
    ${EPOC_LIB}/euser.lib
    ${EPOC_LIB}/estlib.lib
    ${EPOC_LIB}/ws32.lib
    ${EPOC_LIB}/hal.lib
    ${EPOC_LIB}/fbscli.lib
    ${EPOC_LIB}/efsrv.lib
    ${EPOC_LIB}/scdv.lib
    ${EPOC_LIB}/gdi.lib)

  set(Pico-8_libs
    ${EPOC_LIB}/euser.lib
    ${EPOC_LIB}/apparc.lib
    ${EPOC_LIB}/cone.lib
    ${EPOC_LIB}/eikcore.lib
    ${EPOC_LIB}/avkon.lib)

  set(Pico-8_sources
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ngage.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ngage_application.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ngage_appui.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ngage_appview.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ngage_document.cpp")

  add_library(game STATIC ${project_sources})
  add_library(Pico-8 STATIC ${Pico-8_sources})
  build_exe_static(game exe ${UID1} ${UID2} ${UID3} "${game_static_libs}" "${game_libs}")
  build_dll(Pico-8 app ${UID1} ${UID2} ${APP_UID} "${Pico-8_libs}")

  if(NOT BUILD_CI_WORKFLOW)
    build_aif(${CMAKE_CURRENT_SOURCE_DIR}/res Pico-8 ${APP_UID})
    build_resource(${CMAKE_CURRENT_SOURCE_DIR}/res Pico-8 "")
    build_sis(${CMAKE_CURRENT_SOURCE_DIR}/res Pico-8)

    add_dependencies(
      Pico-8.sis
      game.exe
      Pico-8.app
      Pico-8.aif
      Pico-8.rsc)
  endif()

  add_dependencies(
    game.exe
    game)

  target_compile_definitions(
    game
    PUBLIC
    __EXE__
    UID1=${UID1}
    UID2=${UID2}
    UID3=${UID3})

  target_include_directories(
    game
    PUBLIC
    ${SRC_DIR})

  add_dependencies(
    Pico-8.app
    Pico-8)

  target_compile_definitions(
    Pico-8
    PUBLIC
    __DLL__
    UID1=${UID1}
    UID2=${UID2}
    UID3=${APP_UID})

  target_include_directories(
    Pico-8
    PUBLIC
    ${SRC_DIR})

else()

  project(Pico-8 C)

  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/export")

  include(FetchContent)

  FetchContent_Declare(
  sdl3
    URL https://github.com/libsdl-org/SDL/releases/download/release-3.2.4/SDL3-devel-3.2.4-VC.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

  FetchContent_MakeAvailable(sdl3)
  message(STATUS "Extracted files are located in: ${sdl3_SOURCE_DIR}")

  add_library(SDL3 STATIC IMPORTED)

  set_target_properties(SDL3 PROPERTIES
    IMPORTED_LOCATION "${sdl3_SOURCE_DIR}/lib/x86/SDL3.lib")

  set(z8lua_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lapi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lauxlib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lbaselib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lcode.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lcorolib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lctype.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/ldblib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/ldebug.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/ldo.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/ldump.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lfunc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lgc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/linit.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/llex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lmem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lobject.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lopcodes.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lparser.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lpico8lib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lstate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lstring.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lstrlib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/ltable.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/ltablib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/ltm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lundump.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lvm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lzio.c)

  add_executable(Pico-8 ${project_sources} ${z8lua_sources})
  target_link_libraries(Pico-8 PRIVATE SDL3)

  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

  configure_file("${sdl3_SOURCE_DIR}/lib/x86/SDL3.dll" ${CMAKE_CURRENT_SOURCE_DIR}/export COPYONLY)
  configure_file("${sdl3_SOURCE_DIR}/lib/x86/SDL3.pdb" ${CMAKE_CURRENT_SOURCE_DIR}/export COPYONLY)

  include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${sdl3_SOURCE_DIR}/include")

endif()
