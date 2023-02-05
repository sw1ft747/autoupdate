#ifndef __app__H
#define __app__H

#ifdef _WIN32
#pragma once
#endif

#ifdef __cplusplus

extern "C" char app_bytes[];
extern "C" unsigned int app_size;

#else

extern char app_bytes[];
extern unsigned int app_size;

#endif

#endif