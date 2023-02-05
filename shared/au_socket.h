#ifndef __AUTOUPDATE_SOCKET__H
#define __AUTOUPDATE_SOCKET__H

#ifdef _WIN32
#pragma once
#endif

#include "au_platform.h"

#ifdef AU_PLATFORM_WINDOWS
#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#endif

#ifdef AU_PLATFORM_LINUX
#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (~0)
#endif
#endif

//-----------------------------------------------------------------------------
// Some types
//-----------------------------------------------------------------------------

typedef unsigned int socket_t;
typedef sockaddr_in sockaddr_in_t;
typedef sockaddr sockaddr_t;

//-----------------------------------------------------------------------------
// Simple abstraction layer to manage server/client sockets over TCP protocol
//-----------------------------------------------------------------------------

class CAUSocketTCP
{
public:
	CAUSocketTCP();

	socket_t Create(const char *pszIpAddress, unsigned short unPort, sockaddr_in_t *name);

	int Bind(sockaddr_t *name, int namelen);
	int Listen();

	socket_t Accept();
	int Connect(sockaddr_t *name, int namelen);
	int Close(socket_t socket);

	int SetOptions(int level, int optname);

	int Send(socket_t socket, const void *buffer, int len, int flags);
	int Recv(socket_t socket, void *buffer, int len, int flags);

	inline int Bind(sockaddr_in_t *name, int namelen) { return Bind((sockaddr_t *)name, namelen); }

	inline int Connect(sockaddr_in_t *name, int namelen) { return Connect((sockaddr_t *)name, namelen); }
	
	inline int Close() { return Close(m_socket); }
	
	inline int Send(const void *buffer, int len, int flags) { return Send(m_socket, buffer, len, flags); }
	inline int Recv(void *buffer, int len, int flags) { return Recv(m_socket, buffer, len, flags); }

public:
	static void Initialize();
	static void Shutdown();

	static int GetLastError();
	static const char *GetLastErrorString();
	static void FreeErrorString(const char *pszError);
	static void PrintSocketLastError(const char *pszMessage);

private:
	socket_t m_socket;
};

#endif
