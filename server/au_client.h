#ifndef AU_CLIENT_H
#define AU_CLIENT_H

#ifdef _WIN32
#pragma once
#endif

#include "../shared/au_socket.h"

class CAUClient
{
public:
	CAUClient();
	CAUClient(socket_t socket);

	inline void EstablishConnection() { m_bConnected = true; }
	inline bool IsConnected() const { return m_bConnected; }

	inline void SetPlatform(int platform) { m_platform = platform; }
	inline int GetPlatform() const { return m_platform; }

	operator socket_t() const { return m_socket; }

private:
	bool m_bConnected;
	int m_platform;

	socket_t m_socket;
};

#endif