#include "au_client.h"

CAUClient::CAUClient()
{
	m_bConnected = false;
	m_platform = AU_CLIENT_PLATFORM_UNKNOWN;
	m_socket = INVALID_SOCKET;
}

CAUClient::CAUClient(socket_t socket)
{
	m_bConnected = false;
	m_platform = AU_CLIENT_PLATFORM_UNKNOWN;
	m_socket = socket;
}