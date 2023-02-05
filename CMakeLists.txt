cmake_minimum_required(VERSION 3.3.2)

project(AutoUpdater)

set(AUTOUPDATER "autoupdate")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(AUTOUPDATER_SOURCE_FILES
	src/main.cpp
	
	src/au_app_raw.c
	src/au_app_raw.h
	
	shared/au_platform.h
	shared/au_protocol.h
	shared/au_app_info.h
	shared/au_app_version.h
	shared/au_utils.cpp
	shared/au_utils.h
)

add_subdirectory(server)
add_subdirectory(client)

add_executable(${AUTOUPDATER} ${AUTOUPDATER_SOURCE_FILES})

add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)

if (MSVC)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /DNDEBUG")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
else()
	set(CMAKE_CXX_FLAGS "-m32 -g -std=c++14 -w -Wl,--no-as-needed -ldl")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3 -s -DNDEBUG")
	set(CMAKE_CXX_FLAGS_DEBUG "-Os")
	
	set(CMAKE_C_FLAGS "-m32 -g -w -Wl,--no-as-needed -ldl")
	set(CMAKE_C_FLAGS_RELEASE "-O3 -s -DNDEBUG")
	set(CMAKE_C_FLAGS_DEBUG "-Os")
endif()
