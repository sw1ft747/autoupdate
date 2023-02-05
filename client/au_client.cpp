#include <stdio.h>

#include "au_client.h"

#include "../shared/au_protocol.h"
#include "../shared/au_utils.h"

//-----------------------------------------------------------------------------
// Init, Shutdown
//-----------------------------------------------------------------------------

bool CAUClient::Initialize(const char *pszIpAddress, unsigned short unPort)
{
	CAUSocketTCP::Initialize();

	int result;

	result = (int)m_SocketTCP.Create(pszIpAddress, unPort, &m_address);
	
	if ( result == INVALID_SOCKET )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Create");
		return false;
	}
	
	return true;
}

void CAUClient::Shutdown()
{
	m_SocketTCP.Close();

	CAUSocketTCP::Shutdown();
}

//-----------------------------------------------------------------------------
// Communication bridge with server
// Returns 'true' to keep the client thread alive, otherwise 'false' to terminate the thread
//-----------------------------------------------------------------------------

bool CAUClient::Session()
{
	// Never was trying to make something similar, so it may look ugly D:

	bool bUpdateAvailable;
	au_app_version_t version;

	version.major = AUTOUPDATE_APP_MAJOR_VERSION;
	version.minor = AUTOUPDATE_APP_MINOR_VERSION;
	version.patch = AUTOUPDATE_APP_PATCH_VERSION;

	//-----------------------------------------------------------------------------
	// Establish connection with server
	//-----------------------------------------------------------------------------
	
	if ( !EstablishConnection() )
		return false;

	AU_Printf("Connection accepted\n");

	//-----------------------------------------------------------------------------
	// Send to server our platform
	//-----------------------------------------------------------------------------

	if ( !SendPlatformType() )
		return false;

	AU_Printf("Transmitted platform type\n");

	//-----------------------------------------------------------------------------
	// Send app's version to server
	//-----------------------------------------------------------------------------

	if ( !SendAppVersion(version, &bUpdateAvailable) )
		return false;

	AU_Printf("Client's app version is %s\n", bUpdateAvailable ? "outdated" : "up to date");

	//-----------------------------------------------------------------------------
	// Send to server new query
	//-----------------------------------------------------------------------------

	// Nothing to do here
	if ( !bUpdateAvailable )
	{
		Disconnect(AUResultCode_OK);

		AU_Printf("Disconnected from server\n");
		return false;
	}

	// Query update
	AU_Protocol_InitPacket(&m_packet, AU_PACKET_QUERY_UPDATE, 0);

	if ( Socket()->Send(&m_packet, sizeof(m_packet), 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Send <> AU_PACKET_QUERY_UPDATE");
		return false;
	}

	if ( !RecvPacket() )
		return false;

	if ( m_packet.type == AU_PACKET_DISCONNECT )
	{
		int code;

		if ( Socket()->Recv(&code, sizeof(code), 0) == SOCKET_ERROR )
		{
			CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Recv <> AU_PACKET_DISCONNECT");
			return false;
		}

		AU_Printf("Server disconnected with code: %d\n", code);
		return false;
	}

	// Receiving an update
	RecvUpdate();

	AU_Printf("An update has been received\n");

	//-----------------------------------------------------------------------------
	// Session ends with disconnect packet
	//-----------------------------------------------------------------------------

	int code;

	if ( !RecvDisconnect(&code) )
		return false;

	AU_Printf("Server disconnected with code: %d\n", code);

	return false;
}

bool CAUClient::RecvUpdate()
{
	extern unsigned char *g_pUpdateData;
	extern unsigned int g_ulUpdateSize;

	unsigned char key[8];

	g_ulUpdateSize = m_packet.length;
	g_pUpdateData = (unsigned char *)malloc(g_ulUpdateSize);

	if ( g_pUpdateData == NULL )
	{
		AU_Printf("Failed to allocate memory\n");
		return false;
	}

	if ( Socket()->Recv(key, 8, 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Recv <> UPDATE");
		return false;
	}

	int iReceivedBytes = 0;

	while ( iReceivedBytes != g_ulUpdateSize )
	{
		int bytes;

		if ( (bytes = Socket()->Recv(g_pUpdateData + iReceivedBytes, g_ulUpdateSize - iReceivedBytes, 0)) == SOCKET_ERROR )
		{
			CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Recv <> UPDATE");
			return false;
		}

		iReceivedBytes += bytes;
	}
	
	AU_DecryptData(g_pUpdateData, key, g_ulUpdateSize, 8);

	return true;
}

bool CAUClient::EstablishConnection()
{
	unsigned long long unused = 0;
	AU_Protocol_InitPacket(&m_packet, AU_PACKET_ESTABLISH_CONNECTION, sizeof(unused));

	if ( Socket()->Send(&m_packet, sizeof(m_packet), 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Send <> AU_PACKET_ESTABLISH_CONNECTION");
		return false;
	}
	
	if ( Socket()->Send(&unused, sizeof(unused), 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Send <> AU_PACKET_ESTABLISH_CONNECTION");
		return false;
	}

	return IsServerResponseOK();
}

bool CAUClient::Disconnect(int code)
{
	AU_Protocol_InitPacket(&m_packet, AU_PACKET_DISCONNECT, sizeof(code));

	if ( Socket()->Send(&m_packet, sizeof(m_packet), 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Send <> AU_PACKET_DISCONNECT");
		return false;
	}
	
	if ( Socket()->Send(&code, sizeof(code), 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Send <> AU_PACKET_DISCONNECT");
		return false;
	}

	return true;
}

bool CAUClient::RecvDisconnect(int *code)
{
	*code = AUResultCode_Bad;

	if ( Socket()->Recv(&m_packet, sizeof(m_packet), 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Recv <> AU_PACKET_DISCONNECT");
		return false;
	}

	if ( !AU_Protocol_PacketIsValid(&m_packet) )
	{
		AU_Printf("AU_PACKET_DISCONNECT <> Received invalid packet\n");
		return false;
	}

	if ( m_packet.type != AU_PACKET_DISCONNECT )
	{
		AU_Printf("AU_PACKET_DISCONNECT <> Invalid packet type\n");
		return false;
	}

	int response;

	if ( Socket()->Recv(&response, sizeof(response), 0) == SOCKET_ERROR)
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Recv <> AU_PACKET_DISCONNECT");
		return false;
	}

	*code = response;

	return true;
}

bool CAUClient::RecvPacket()
{
	if ( Socket()->Recv(&m_packet, sizeof(m_packet), 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Recv <> RecvPacket");
		return false;
	}

	if ( !AU_Protocol_PacketIsValid(&m_packet) )
	{
		AU_Printf("RecvPacket <> Received invalid packet\n");
		return false;
	}

	return true;
}

bool CAUClient::SendPlatformType()
{
	int platform;

#ifdef AU_PLATFORM_WINDOWS
	platform = AU_CLIENT_PLATFORM_WINDOWS;
#else
	platform = AU_CLIENT_PLATFORM_LINUX;
#endif

	AU_Protocol_InitPacket(&m_packet, AU_PACKET_PLATFORM, sizeof(platform));

	if ( Socket()->Send(&m_packet, sizeof(m_packet), 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Send <> AU_PACKET_PLATFORM");
		return false;
	}
	
	if ( Socket()->Send(&platform, sizeof(platform), 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Send <> AU_PACKET_PLATFORM");
		return false;
	}

	return IsServerResponseOK();
}

bool CAUClient::SendAppVersion(au_app_version_t &version, bool *bUpdateAvailable)
{
	*bUpdateAvailable = false;

	AU_Protocol_InitPacket(&m_packet, AU_PACKET_APP_VERSION, sizeof(version));

	if ( Socket()->Send(&m_packet, sizeof(m_packet), 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Send <> AU_PACKET_APP_VERSION");
		return false;
	}

	if ( Socket()->Send(&version, sizeof(version), 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Send <> AU_PACKET_APP_VERSION");
		return false;
	}

	int result, response;
	
	response = RecvServerResponse( &result );

	if ( result == AUResultCode_Bad )
		return false;

	if ( response == AUResultCode_UpdateAvailable )
		*bUpdateAvailable = true;
	else if ( response != AUResultCode_OK )
		return false;

	return true;
}

bool CAUClient::IsServerResponseOK()
{
	int type;
	int length;

	if ( Socket()->Recv(&m_packet, sizeof(m_packet), 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Recv <> AU_PACKET_RESPONSE");
		return false;
	}

	if ( !AU_Protocol_PacketIsValid(&m_packet) )
	{
		AU_Printf("AU_PACKET_RESPONSE <> Received invalid packet\n");
		return false;
	}

	type = m_packet.type;
	length = m_packet.length;

	if ( type != AU_PACKET_RESPONSE )
	{
		AU_Printf("AU_PACKET_RESPONSE <> Invalid packet type\n");
		return false;
	}

	int response;

	if ( Socket()->Recv(&response, length, 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Recv <> AU_PACKET_RESPONSE");
		return false;
	}

	if ( response != AUResultCode_OK )
	{
		AU_Printf("AU_PACKET_RESPONSE received error code %d\n", response);
		return false;
	}

	return true;
}

int CAUClient::RecvServerResponse(int *result)
{
	int type;
	int length;

	*result = AUResultCode_OK;

	if ( Socket()->Recv(&m_packet, sizeof(m_packet), 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Recv <> AU_PACKET_RESPONSE");
		*result = AUResultCode_SocketError;
		
		return AUResultCode_Bad;
	}

	if ( !AU_Protocol_PacketIsValid(&m_packet) )
	{
		AU_Printf("AU_PACKET_RESPONSE <> Received invalid packet\n");
		*result = AUResultCode_InvalidPacket;
		
		return AUResultCode_Bad;
	}

	type = m_packet.type;
	length = m_packet.length;

	if ( type != AU_PACKET_RESPONSE )
	{
		AU_Printf("AU_PACKET_RESPONSE <> Invalid packet type\n");
		*result = AUResultCode_InvalidPacketType;
		
		return AUResultCode_Bad;
	}

	int response;

	if ( Socket()->Recv(&response, length, 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Recv <> AU_PACKET_RESPONSE");
		*result = AUResultCode_SocketError;
		
		return AUResultCode_Bad;
	}

	return response;
}