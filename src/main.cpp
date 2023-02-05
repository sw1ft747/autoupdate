#include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
#include <conio.h>
#endif

#include "au_app_raw.h"

#include "../shared/au_app_info.h"
#include "../shared/au_platform.h"
#include "../shared/au_utils.h"

void UpdateApp()
{
	AU_Printf("\nUpdate has been confirmed.\n");
	AU_Printf("\nUnpacking...\n");

	FILE *file = fopen(AUTOUPDATE_APP_FILENAME, "wb");

	if ( file != NULL )
	{
		fwrite( app_bytes, sizeof(app_bytes[0]), app_size, file);
		fclose( file );

		AU_Printf("\nFinished unpacking.\n");
	}
	else
	{
		AU_Printf("\nUnable to replace an app.\n");
	}
}

void SelfDelete(char *argv[])
{
#ifdef AU_PLATFORM_WINDOWS
	char szModuleName[MAX_PATH];
	char szCmd[512];

	STARTUPINFO startupInfo = { 0 };
	PROCESS_INFORMATION procInfo = { 0 };

	GetModuleFileName(NULL, szModuleName, MAX_PATH);

	sprintf(szCmd, "cmd.exe /C -w 1000 > Nul & Del /f /q \"%s\"", szModuleName);

	CreateProcess(NULL, szCmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &startupInfo, &procInfo);

	CloseHandle( procInfo.hThread );
	CloseHandle( procInfo.hProcess );
#else
	remove( argv[0] );
#endif
}

int main(int argc, char *argv[])
{
	char c;

	AU_Printf("An update for \"" AUTOUPDATE_APP_NAME "\" is available. Version: " AUTOUPDATE_APP_VERSION_STRING "\n\n");
	AU_Printf("Proceed update? [Y/N]: ");

	if ( scanf("%c", &c) && (c == 'y' || c == 'Y') )
	{
		UpdateApp();
	}
	else
	{
		AU_Printf("\nUpdate has been canceled.\n");
	}

	AU_Printf("\nPress any key to exit.\n");

#pragma warning(push)
#pragma warning(disable : 6031)

#ifdef AU_PLATFORM_WINDOWS
	getch();
#else
	getchar();
#endif

#pragma warning(pop)

	SelfDelete( argv );

	return 0;
}
