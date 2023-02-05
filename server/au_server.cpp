#include <stdio.h>

#include "au_server.h"

#include "../shared/au_protocol.h"
#include "../shared/au_utils.h"

bool CAUServer::Initialize(const char *pszIpAddress, unsigned short unPort)
{
	CAUSocketTCP::Initialize();

	int result;

	result = (int)m_SocketTCP.Create(pszIpAddress, unPort, &m_address);
	
	if ( result == INVALID_SOCKET )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Create");
		return false;
	}
	
	result = m_SocketTCP.Bind(&m_address, sizeof(m_address));
	
	if ( result == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Bind");
		return false;
	}
	
	result = m_SocketTCP.Listen();
	
	if ( result == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Listen");
		return false;
	}

	return true;
}

void CAUServer::Shutdown()
{
	m_SocketTCP.SetOptions(SOL_SOCKET, SO_REUSEADDR);
	m_SocketTCP.Close();

	CAUSocketTCP::Shutdown();
}

//-----------------------------------------------------------------------------
// Handler of all connected clients
// Returns 'true' to keep the client thread alive, otherwise 'false' to terminate the thread
//-----------------------------------------------------------------------------

bool CAUServer::ClientSession(CAUClient &client)
{
	extern int g_iMajorVersion;
	extern int g_iMinorVersion;
	extern int g_iPatchVersion;

	au_app_version_t app_version;
	au_app_version_t client_app_version;

	app_version.major = g_iMajorVersion;
	app_version.minor = g_iMinorVersion;
	app_version.patch = g_iPatchVersion;

	socket_t sock = client;

	//-----------------------------------------------------------------------------
	// Establish connection with client
	//-----------------------------------------------------------------------------

	if ( !EstablishConnection(client) )
		return false;

	AU_Printf("<%d> Established connection with client\n", sock);

	//-----------------------------------------------------------------------------
	// Receive client's platform
	//-----------------------------------------------------------------------------
	
	if ( !RecvPlatformType(client) )
		return false;
	
	const char *pszPlatform = ( client.GetPlatform() == AU_CLIENT_PLATFORM_WINDOWS ? "Windows" : client.GetPlatform() == AU_CLIENT_PLATFORM_LINUX ? "Linux" : "Unknown" );
	AU_Printf("<%d> Client's platform: %s\n", sock, pszPlatform);

	//-----------------------------------------------------------------------------
	// Receive app's version from client
	//-----------------------------------------------------------------------------

	if ( !RecvAppVersion(client, client_app_version) )
		return false;

	bool bOutdatedVersion = IsVersionOutdated(app_version, client_app_version);

	AU_Printf("<%d> Client's app version: %d.%d.%d | Server's app version: %d.%d.%d\n", sock, AU_APP_VERSION_EXPAND(client_app_version), AU_APP_VERSION_EXPAND(app_version));
	AU_Printf("<%d> Client's app version is %s\n", sock, bOutdatedVersion ? "outdated" : "up to date");

	if ( !SendResponse(client, bOutdatedVersion ? AUResultCode_UpdateAvailable : AUResultCode_OK) )
		return false;

	//-----------------------------------------------------------------------------
	// Receive new client's query
	//-----------------------------------------------------------------------------

	int type;
	int length;

	// Receive a packet from client
	if ( Socket()->Recv(client, &m_packet, sizeof(m_packet), 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Recv <> NEW PACKET");
		return false;
	}

	if ( !AU_Protocol_PacketIsValid(&m_packet) )
	{
		AU_Printf("NEW PACKET <> Received invalid packet\n");
		return false;
	}

	type = m_packet.type;
	length = m_packet.length;

	// Client disconnected
	if ( type == AU_PACKET_DISCONNECT )
	{
		int code;

		if ( Socket()->Recv(client, &code, sizeof(code), 0) == SOCKET_ERROR )
		{
			CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Recv <> AU_PACKET_DISCONNECT");
			return false;
		}

		AU_Printf("<%d> Client disconnected with code: %d\n", sock, code);
		return false;
	}
	else if ( type != AU_PACKET_QUERY_UPDATE )
	{
		Disconnect(client, AUResultCode_Bad);

		AU_Printf("<%d> Client sent incorrect packet, disconnecting...\n", sock);
		return false;
	}

	if ( !bOutdatedVersion )
	{
		Disconnect(client, AUResultCode_Bad);
		AU_Printf("<%d> Client queried an update, but their version is already up to date\n", sock);
		return false;
	}

	// Sending an update
	SendUpdate(client);

	AU_Printf("<%d> An update has been transmitted to client\n", sock);

	//-----------------------------------------------------------------------------
	// Session has ended
	//-----------------------------------------------------------------------------

	Disconnect(client, AUResultCode_OK);
	AU_Printf("<%d> Client has ended session\n", sock);

	return false;
}

bool CAUServer::SendUpdate(CAUClient &client)
{
	AU_AUTO_LOCK( m_UpdateMutex );

	extern unsigned char *g_pWindowsFile;
	extern unsigned char *g_pLinuxFile;
	extern unsigned int g_ulWindowsFileSize;
	extern unsigned int g_ulLinuxFileSize;

	unsigned char key[8];

	unsigned char *pUpdateFile = ( client.GetPlatform() == AU_CLIENT_PLATFORM_WINDOWS ? g_pWindowsFile : g_pLinuxFile );
	unsigned int ulUpdateFileSize = ( client.GetPlatform() == AU_CLIENT_PLATFORM_WINDOWS ? g_ulWindowsFileSize : g_ulLinuxFileSize );

	// Generate random key
	for (int i = 0; i < 8; i++)
	{
		key[i] = AU_RandomInt( 0, 255 );
	}

	AU_Protocol_InitPacket(&m_packet, AU_PACKET_QUERY_UPDATE, ulUpdateFileSize);

	if ( Socket()->Send(client, &m_packet, sizeof(m_packet), 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Send <> AU_PACKET_QUERY_UPDATE");
		return false;
	}

	if ( !AU_Protocol_PacketIsValid(&m_packet) )
	{
		AU_Printf("AU_PACKET_QUERY_UPDATE <> Received invalid packet\n");
		return false;
	}

	AU_EncryptData(pUpdateFile, key, ulUpdateFileSize, 8);

	if ( Socket()->Send(client, key, 8, 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Send <> AU_PACKET_QUERY_UPDATE");
		return false;
	}

	int iTransmittedBytes = 0;

	while ( iTransmittedBytes != ulUpdateFileSize )
	{
		int bytes;

		if ( (bytes = Socket()->Send(client, pUpdateFile + iTransmittedBytes, ulUpdateFileSize - iTransmittedBytes, 0)) == SOCKET_ERROR )
		{
			CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Send <> AU_PACKET_QUERY_UPDATE");
			return false;
		}

		iTransmittedBytes += bytes;
	}
	
	AU_DecryptData(pUpdateFile, key, ulUpdateFileSize, 8);

	return true;
}

bool CAUServer::EstablishConnection(CAUClient &client)
{
	int type;
	int length;

	unsigned long long unused;

	if ( Socket()->Recv(client, &m_packet, sizeof(m_packet), 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Recv <> AU_PACKET_ESTABLISH_CONNECTION");
		return false;
	}

	if ( !AU_Protocol_PacketIsValid(&m_packet) )
	{
		AU_Printf("AU_PACKET_ESTABLISH_CONNECTION <> Received invalid packet\n");
		return false;
	}

	type = m_packet.type;
	length = m_packet.length;

	if ( type != AU_PACKET_ESTABLISH_CONNECTION )
	{
		AU_Printf("AU_PACKET_ESTABLISH_CONNECTION <> Invalid packet type\n");
		return false;
	}

	if ( Socket()->Recv(client, &unused, sizeof(unused), 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Recv <> AU_PACKET_ESTABLISH_CONNECTION");
		return false;
	}

	if ( !SendResponse(client, AUResultCode_OK) )
		return false;

	client.EstablishConnection();
	return true;
}

bool CAUServer::Disconnect(CAUClient &client, int code)
{
	AU_Protocol_InitPacket(&m_packet, AU_PACKET_DISCONNECT, sizeof(code));

	if ( Socket()->Send(client, &m_packet, sizeof(m_packet), 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Send <> AU_PACKET_DISCONNECT");
		return false;
	}
	
	if ( Socket()->Send(client, &code, sizeof(code), 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Send <> AU_PACKET_DISCONNECT");
		return false;
	}

	return true;
}

bool CAUServer::RecvPlatformType(CAUClient &client)
{
	int type;
	int length;

	int platform;

	if ( Socket()->Recv(client, &m_packet, sizeof(m_packet), 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Recv <> AU_PACKET_PLATFORM");
		return false;
	}

	if ( !AU_Protocol_PacketIsValid(&m_packet) )
	{
		AU_Printf("AU_PACKET_PLATFORM <> Received invalid packet\n");
		return false;
	}

	type = m_packet.type;
	length = m_packet.length;

	if ( type != AU_PACKET_PLATFORM )
	{
		AU_Printf("AU_PACKET_PLATFORM <> Invalid packet type\n");
		return false;
	}

	if ( Socket()->Recv(client, &platform, sizeof(platform), 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Recv <> AU_PACKET_PLATFORM");
		return false;
	}

	if ( platform != AU_CLIENT_PLATFORM_WINDOWS && platform != AU_CLIENT_PLATFORM_LINUX )
	{
		SendResponse(client, AUResultCode_BadPlatform);
		return false;
	}

	if ( !SendResponse(client, AUResultCode_OK) )
		return false;

	client.SetPlatform(platform);
	return true;
}

bool CAUServer::RecvAppVersion(CAUClient &client, au_app_version_t &client_app_version)
{
	int type;
	int length;

	if ( Socket()->Recv(client, &m_packet, sizeof(m_packet), 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Recv <> AU_PACKET_APP_VERSION");
		return false;
	}

	if ( !AU_Protocol_PacketIsValid(&m_packet) )
	{
		AU_Printf("AU_PACKET_APP_VERSION <> Received invalid packet\n");
		return false;
	}

	type = m_packet.type;
	length = m_packet.length;

	if ( type != AU_PACKET_APP_VERSION )
	{
		AU_Printf("AU_PACKET_APP_VERSION <> Invalid packet type\n");
		return false;
	}

	if ( Socket()->Recv(client, &client_app_version, sizeof(client_app_version), 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Recv <> AU_PACKET_APP_VERSION");
		return false;
	}

	return true;
}

bool CAUServer::SendResponse(CAUClient &client, int code)
{
	int response = code;
	AU_Protocol_InitPacket(&m_packet, AU_PACKET_RESPONSE, sizeof(response));

	if ( Socket()->Send(client, &m_packet, sizeof(m_packet), 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Send <> AU_PACKET_RESPONSE");
		return false;
	}
	
	if ( Socket()->Send(client, &response, sizeof(response), 0) == SOCKET_ERROR )
	{
		CAUSocketTCP::PrintSocketLastError("CAUSocketTCP::Send <> AU_PACKET_RESPONSE");
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Check if client version is outdated
//-----------------------------------------------------------------------------

bool CAUServer::IsVersionOutdated(au_app_version_t &server, au_app_version_t &client)
{
	if ( server.major > client.major )
	{
		return true;
	}
	else if ( server.minor > client.minor )
	{
		return true;
	}
	else if ( server.patch > client.patch )
	{
		return true;
	}

	return false;
}