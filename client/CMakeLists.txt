cmake_minimum_required(VERSION 3.3.2)

project(AutoUpdaterClient)

set(AUTOUPDATER_CLIENT "autoupdate_client")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(AUTOUPDATER_CLIENT_SOURCE_FILES
	${CMAKE_SOURCE_DIR}/client/main.cpp
	${CMAKE_SOURCE_DIR}/client/au_client.cpp
	${CMAKE_SOURCE_DIR}/client/au_client.h
	
	${CMAKE_SOURCE_DIR}/shared/au_platform.h
	${CMAKE_SOURCE_DIR}/shared/au_app_info.h
	${CMAKE_SOURCE_DIR}/shared/au_app_version.h
	${CMAKE_SOURCE_DIR}/shared/au_protocol.cpp
	${CMAKE_SOURCE_DIR}/shared/au_protocol.h
	${CMAKE_SOURCE_DIR}/shared/au_socket.cpp
	${CMAKE_SOURCE_DIR}/shared/au_socket.h
	${CMAKE_SOURCE_DIR}/shared/au_utils.cpp
	${CMAKE_SOURCE_DIR}/shared/au_utils.h
)

add_executable(${AUTOUPDATER_CLIENT} ${AUTOUPDATER_CLIENT_SOURCE_FILES})

add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE -D_WINSOCK_DEPRECATED_NO_WARNINGS)

if (MSVC)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /DNDEBUG")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
else()
	set(CMAKE_CXX_FLAGS "-m32 -g -std=c++14 -w -Wl,--no-as-needed -ldl -pthread")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3 -s -DNDEBUG")
	set(CMAKE_CXX_FLAGS_DEBUG "-Os")
endif()
