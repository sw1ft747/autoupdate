#ifndef AU_CLIENT_H
#define AU_CLIENT_H

#ifdef _WIN32
#pragma once
#endif

#include "../shared/au_socket.h"
#include "../shared/au_protocol.h"

#include "../shared/au_app_info.h"
#include "../shared/au_app_version.h"

class CAUClient
{
public:
	bool Initialize(const char *pszIpAddress, unsigned short unPort);
	void Shutdown();

	bool Session();

	bool EstablishConnection();
	bool Disconnect(int code);
	bool RecvDisconnect(int *code);
	bool RecvPacket();
	bool SendPlatformType();
	bool SendAppVersion(au_app_version_t &version, bool *bUpdateAvailable);
	bool IsServerResponseOK();
	int RecvServerResponse(int *result);
	bool RecvUpdate();

	inline CAUSocketTCP *Socket() { return &m_SocketTCP; };
	inline sockaddr_in_t *Address() { return &m_address; };

private:
	bool m_bConnected;

	CAUSocketTCP m_SocketTCP;
	sockaddr_in_t m_address;
	AUPacket m_packet;
};

#endif