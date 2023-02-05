## generate_app_info.pl / (c) Sw1ft
# A simple Perl script that converts a given app name, file name and version of format <MAJOR.MINOR.PATCH> to a C/C++ header file
# Usage: perl generate_app_info.pl <appname> <filename> <major version> <minor version> <patch version>
#

# Arguments
$name = $ARGV[0];
$filename = $ARGV[1];
$major = $ARGV[2];
$minor = $ARGV[3];
$patch  = $ARGV[4];

if ( open(hOutput, ">shared/au_app_info.h") )
{
	printf( hOutput "#ifndef __AUTOUPDATE_APP_INFO__H\n#define __AUTOUPDATE_APP_INFO__H\n\n#ifdef _WIN32\n#pragma once\n#endif\n\n" );
	printf( hOutput "#define AUTOUPDATE_APP_NAME \"%s\"\n", $name );
	printf( hOutput "#define AUTOUPDATE_APP_FILENAME \"%s\"\n\n", $filename );
	printf( hOutput "#define AUTOUPDATE_APP_MAJOR_VERSION ( %d )\n#define AUTOUPDATE_APP_MINOR_VERSION ( %d )\n#define AUTOUPDATE_APP_PATCH_VERSION ( %d )\n\n", int($major), int($minor), int($patch) );
	printf( hOutput "#define AUTOUPDATE_APP_MAJOR_VERSION_STRING \"%d\"\n#define AUTOUPDATE_APP_MINOR_VERSION_STRING \"%d\"\n#define AUTOUPDATE_APP_PATCH_VERSION_STRING \"%d\"\n\n", int($major), int($minor), int($patch) );
	printf( hOutput "#define AUTOUPDATE_APP_VERSION_STRING AUTOUPDATE_APP_MAJOR_VERSION_STRING \".\" AUTOUPDATE_APP_MINOR_VERSION_STRING \".\" AUTOUPDATE_APP_PATCH_VERSION_STRING\n\n#endif" );
	
	close( hOutput );
}

system( "perl bin2c.pl file/" . $filename . " src/au_app_raw app" );