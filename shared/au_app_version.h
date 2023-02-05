#ifndef __AUTOUPDATE_APP_VERSION__H
#define __AUTOUPDATE_APP_VERSION__H

#ifdef _WIN32
#pragma once
#endif

#define AU_APP_VERSION_EXPAND(ver) (ver.major), (ver.minor), (ver.patch)

struct au_app_version_t
{
	int major;
	int minor;
	int patch;
};

#endif