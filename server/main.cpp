#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../shared/au_platform.h"
#include "../shared/au_socket.h"
#include "../shared/au_utils.h"

#include "au_client.h"
#include "au_server.h"

#ifdef AU_PLATFORM_WINDOWS
#include <process.h>
#else
#include <pthread.h>
#include <strings.h>
#endif

#ifdef AU_PLATFORM_LINUX
#define stricmp strcasecmp
#endif

CAUServer server;

// Settings
const char *g_pszIpAddress = NULL;
unsigned short g_unPort = 0;
const char *g_pszWindowsFile = NULL;
const char *g_pszLinuxFile = NULL;
int g_iMajorVersion = -1;
int g_iMinorVersion = -1;
int g_iPatchVersion = -1;

// Auto update files
unsigned char *g_pWindowsFile = NULL;
unsigned char *g_pLinuxFile = NULL;
unsigned int g_ulWindowsFileSize = 0;
unsigned int g_ulLinuxFileSize = 0;

#ifdef AU_PLATFORM_WINDOWS
static unsigned int __stdcall ClientSession(void *lpParam)
#else
static void *ClientSession(void *lpParam)
#endif
{
	CAUClient client( (socket_t)lpParam );

	while ( 1 )
	{
		bool bIsActive = server.ClientSession(client);

		if ( !bIsActive )
		{
			server.Socket()->Close(client);
			break;
		}
	}

	return 0;
}

static bool LoadSettingsFromFile(const char *pszFilename)
{
	char szBuffer[1024];
	FILE *file = fopen(pszFilename, "r");

	if ( file != NULL )
	{
		int nLine = 0;

		while ( fgets(szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]), file))
		{
			nLine++;

			char *buffer = AU_ParseUtil_LStrip(szBuffer);
			AU_ParseUtil_RemoveComment(buffer);
			AU_ParseUtil_RStrip(buffer);

			if ( !*buffer )
				continue;

			char *key = buffer;
			char *value = NULL;

			char *tmp = buffer;

			while ( *tmp )
			{
				if ( *tmp == '=' )
				{
					value = tmp + 1;
					*tmp = '\0';
					break;
				}

				tmp++;
			}

			if ( value == NULL )
			{
				AU_Printf("Expected a key-value pair separated by symbol '=' in settings file \"%s\" (%d)\n", pszFilename, nLine);
				continue;
			}

			const char *pszKey = AU_ParseUtil_Strip(key);
			const char *pszValue = AU_ParseUtil_Strip(value);

			if ( !stricmp(pszKey, "ip") )
			{
				g_pszIpAddress = strdup(pszValue);
			}
			else if ( !stricmp(pszKey, "port") )
			{
				g_unPort = atoi(pszValue) & 0xFFFF;
			}
			else if ( !stricmp(pszKey, "windows_file") )
			{
				g_pszWindowsFile = strdup(pszValue);
			}
			else if ( !stricmp(pszKey, "linux_file") )
			{
				g_pszLinuxFile = strdup(pszValue);
			}
			else if ( !stricmp(pszKey, "major_version") )
			{
				g_iMajorVersion = atoi(pszValue);
			}
			else if ( !stricmp(pszKey, "minor_version") )
			{
				g_iMinorVersion = atoi(pszValue);
			}
			else if ( !stricmp(pszKey, "patch_version") )
			{
				g_iPatchVersion = atoi(pszValue);
			}
		}

		fclose( file );

		if ( g_pszIpAddress == NULL )
		{
			AU_Printf("IP address not given in settings file \"%s\"\n", pszFilename);
			return false;
		}
		else if ( g_unPort == 0 )
		{
			AU_Printf("Port not given in settings file \"%s\"\n", pszFilename);
			return false;
		}
		else if ( g_pszWindowsFile == NULL )
		{
			AU_Printf("Windows binary file not given in settings file \"%s\"\n", pszFilename);
			return false;
		}
		else if ( g_pszLinuxFile == NULL )
		{
			AU_Printf("Linux binary file not given in settings file \"%s\"\n", pszFilename);
			return false;
		}
		else if ( g_iMajorVersion < 0 )
		{
			AU_Printf("Major version not given in settings file \"%s\"\n", pszFilename);
			return false;
		}
		else if ( g_iMinorVersion < 0 )
		{
			AU_Printf("Minor version not given in settings file \"%s\"\n", pszFilename);
			return false;
		}
		else if ( g_iPatchVersion < 0 )
		{
			AU_Printf("Patch version not given in settings file \"%s\"\n", pszFilename);
			return false;
		}

		AU_Printf("[Settings] IP address: %s\n", g_pszIpAddress);
		AU_Printf("[Settings] Port: %hu\n", g_unPort);
		AU_Printf("[Settings] Windows file: %s\n", g_pszWindowsFile);
		AU_Printf("[Settings] Linux file: %s\n", g_pszLinuxFile);
		AU_Printf("[Settings] App version: %d.%d.%d\n", g_iMajorVersion, g_iMinorVersion, g_iPatchVersion);

		return true;
	}
	else
	{
		AU_Printf("Missing file \"%s\" to open\n", pszFilename);
	}

	return false;
}

static bool LoadUpdateIntoBuffer()
{
	FILE *win_file = fopen(g_pszWindowsFile, "rb");
	FILE *linux_file = fopen(g_pszLinuxFile, "rb");

	if ( win_file == NULL )
	{
		if ( linux_file != NULL )
			fclose( linux_file );

		AU_Printf("Windows binary file \"%s\" not found on the disk\n", g_pszWindowsFile);
		return false;
	}
	
	if ( linux_file == NULL )
	{
		if ( win_file != NULL )
			fclose( win_file );

		AU_Printf("Linux binary file \"%s\" not found on the disk\n", g_pszLinuxFile);
		return false;
	}

	unsigned int win_file_size;
	unsigned int linux_file_size;

	// get size of win file
	fseek(win_file, 0, SEEK_END);
	win_file_size = ftell(win_file);
	fseek(win_file, 0, SEEK_SET); // back to start

	// get size of linux file
	fseek(linux_file, 0, SEEK_END);
	linux_file_size = ftell(linux_file);
	fseek(linux_file, 0, SEEK_SET);

	if ( win_file_size == 0 || linux_file_size == 0 )
	{
		fclose( win_file );
		fclose( linux_file );

		if ( win_file_size == 0 )
			AU_Printf("Windows binary file \"%s\" is empty\n", g_pszWindowsFile);
		
		if ( linux_file_size == 0 )
			AU_Printf("Linux binary file \"%s\" is empty\n", g_pszLinuxFile);
		
		return false;
	}

	g_pWindowsFile = (unsigned char *)malloc( win_file_size );
	g_pLinuxFile = (unsigned char *)malloc( linux_file_size );

	g_ulWindowsFileSize = win_file_size;
	g_ulLinuxFileSize = linux_file_size;

	if ( g_pWindowsFile == NULL || g_pLinuxFile == NULL )
	{
		fclose( win_file );
		fclose( linux_file );

		AU_Printf("Failed to allocate memory\n");
		return false;
	}

	fread( g_pWindowsFile, sizeof(unsigned char), g_ulWindowsFileSize, win_file );
	fread( g_pLinuxFile, sizeof(unsigned char), g_ulLinuxFileSize, linux_file );

	fclose( win_file );
	fclose( linux_file );

	return true;
}

static void FreeSettings()
{
	if ( g_pszIpAddress != NULL )
		free( (void *)g_pszIpAddress );
	
	if ( g_pszWindowsFile != NULL )
		free( (void *)g_pszWindowsFile );
	
	if ( g_pszLinuxFile != NULL )
		free( (void *)g_pszLinuxFile );

	g_pszIpAddress = NULL;
	g_unPort = 0;
	g_pszWindowsFile = NULL;
	g_pszLinuxFile = NULL;
}

int main(int argc, char *argv[])
{
	int exit_code = EXIT_FAILURE;

	AU_SetSeed( static_cast<unsigned int>( time(NULL) ) );

	if ( LoadSettingsFromFile("autoupdate_server.txt") )
	{
		if ( LoadUpdateIntoBuffer() )
		{
			if ( server.Initialize(g_pszIpAddress, g_unPort) )
			{
				while ( 1 )
				{
					socket_t client = INVALID_SOCKET;

					do
					{
						client = server.Socket()->Accept();

						if ( client != INVALID_SOCKET )
						{
							AU_Printf("<%d> Client connected\n", client);

						#ifdef AU_PLATFORM_WINDOWS
							unsigned int ulThreadId;
							_beginthreadex(NULL, 0, &ClientSession, (void *)client, 0, &ulThreadId);
						#else
							pthread_t clientThread;
							pthread_create(&clientThread, NULL, ClientSession, (void *)client);
						#endif
						}
					}
					while ( client == INVALID_SOCKET );
				}

				server.Shutdown();
				exit_code = EXIT_SUCCESS;
			}
		}

		FreeSettings();
	}
	
#ifdef AU_PLATFORM_WINDOWS
	system("pause");
#endif

	return exit_code;
}
