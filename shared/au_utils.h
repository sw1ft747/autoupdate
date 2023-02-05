#ifndef __AUTOUPDATE_UTILS__H
#define __AUTOUPDATE_UTILS__H

#ifdef _WIN32
#pragma once
#endif

#include <mutex>

#define AU_AUTO_LOCK(mutex) CAUMutexAutoLock __##mutex##__autolock(mutex)

class CAUMutexAutoLock
{
public:
	CAUMutexAutoLock(std::mutex &mutex);
	~CAUMutexAutoLock();

private:
	std::mutex *m_mutex;
};

void AU_Printf(const char *pszMessageFormat, ...);

void AU_SetSeed(unsigned int seed);
int AU_RandomInt(int min, int max);

void AU_EncryptData(unsigned char *data, unsigned char *key, int data_length, int key_length);
void AU_DecryptData(unsigned char *data, unsigned char *key, int data_length, int key_length);

void AU_ParseUtil_RemoveComment(char *str);
char *AU_ParseUtil_LStrip(char *str);
void AU_ParseUtil_RStrip(char *str);
char *AU_ParseUtil_Strip(char *str);

#endif