#ifndef __AUTOUPDATE_PLATFORM__H
#define __AUTOUPDATE_PLATFORM__H

#ifdef _WIN32
#pragma once
#endif

#ifdef _WIN32
#define AU_PLATFORM_WINDOWS
#else
#define AU_PLATFORM_LINUX
#endif

#define AU_CLIENT_PLATFORM_WINDOWS	( 0 )
#define AU_CLIENT_PLATFORM_LINUX	( 1 )
#define AU_CLIENT_PLATFORM_UNKNOWN	( -1 )

#endif