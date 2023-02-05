#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <random>

#include "au_utils.h"

#ifdef AU_SERVER
static std::mutex print_mutex;
#endif

static constexpr auto AU_STRIP_CHARS = " \t\n";
static constexpr auto AU_STRIP_CHARS_LEN = sizeof(AU_STRIP_CHARS) - 1;

CAUMutexAutoLock::CAUMutexAutoLock(std::mutex &mutex)
{
	m_mutex = &mutex;
	m_mutex->lock();
}

CAUMutexAutoLock::~CAUMutexAutoLock()
{
	m_mutex->unlock();
}

void AU_Printf(const char *pszMessageFormat, ...)
{
#ifdef AU_SERVER
	AU_AUTO_LOCK( print_mutex );
#endif

	static char szFormattedMessage[2048] = { 0 };

	va_list args;
	va_start(args, pszMessageFormat);
	vsnprintf(szFormattedMessage, sizeof(szFormattedMessage) / sizeof(szFormattedMessage[0]), pszMessageFormat, args);
	va_end(args);

	printf(szFormattedMessage);
}

void AU_SetSeed(unsigned int seed)
{
	srand( seed );
}

int AU_RandomInt(int min, int max)
{
	return min + ( rand() % (max - min + 1) );
}

// Simple junkie XOR encryption
void AU_EncryptData(unsigned char *data, unsigned char *key, int data_length, int key_length)
{
	int cycle = 0;

	for (int i = 0; i < data_length; i++)
	{
		if (cycle >= key_length)
			cycle = 0;

		data[i] ^= key[cycle++];
	}
}

// Same as EncryptData
void AU_DecryptData(unsigned char *data, unsigned char *key, int data_length, int key_length)
{
	int cycle = 0;

	for (int i = 0; i < data_length; i++)
	{
		if (cycle >= key_length)
			cycle = 0;

		data[i] ^= key[cycle++];
	}
}

static inline bool AU_ContainsChars(char ch, const char *chars, size_t length)
{
	for (size_t i = 0; i < length; ++i)
	{
		if ( chars[i] == ch )
			return true;
	}

	return false;
}

void AU_ParseUtil_RemoveComment(char *str)
{
	char *comment = NULL;

	while (*str)
	{
		if (*str == '#')
		{
			if ( comment == NULL )
				comment = str;
		}

		str++;
	}

	if ( comment != NULL )
	{
		*comment = '\0';
	}
}

char *AU_ParseUtil_LStrip(char *str)
{
	while (*str && AU_ContainsChars(*str, AU_STRIP_CHARS, AU_STRIP_CHARS_LEN))
		++str;

	return str;
}

void AU_ParseUtil_RStrip(char *str)
{
	char *end = str + strlen(str) - 1;

	if (end < str)
		return;

	while (end >= str && AU_ContainsChars(*end, AU_STRIP_CHARS, AU_STRIP_CHARS_LEN))
	{
		*end = '\0';
		--end;
	}
}

char *AU_ParseUtil_Strip(char *str)
{
	char *result = AU_ParseUtil_LStrip(str);
	AU_ParseUtil_RStrip(result);

	return result;
}