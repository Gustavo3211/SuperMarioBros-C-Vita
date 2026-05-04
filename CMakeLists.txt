cmake_minimum_required(VERSION 3.5)
project(smbc)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-narrowing")

# add_subdirectory(codegen)

set(SOURCE_FILES
    source/Configuration.cpp
    source/Main.cpp
    source/Emulation/APU.cpp
    source/Emulation/Controller.cpp
    source/Emulation/MemoryAccess.cpp
    source/Emulation/PPU.cpp
    source/SMB/SMB.cpp
    source/SMB/SMBData.cpp
    source/SMB/SMBEngine.cpp
    source/Util/Video.cpp)

find_package(SDL2 REQUIRED)

add_executable(smbc ${SOURCE_FILES})
target_link_libraries(smbc ${SDL2_LIBRARIES})
# add_dependencies(smbc codegen)

if(DEFINED ENV{VITASDK})
    include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
    vita_create_self(smbc.self smbc)
    vita_create_vpk(smbc.vpk "SMBV00001" smbc.self
        NAME "Super Mario Bros. C"
        VERSION "01.00"
    )
endif()
