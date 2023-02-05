#ifndef __AUTOUPDATE_PROTOCOL__H
#define __AUTOUPDATE_PROTOCOL__H

#ifdef _WIN32
#pragma once
#endif

#define AU_PROTOCOL_VERSION ( 1 )

// Error/result codes
#define AUResultCode_Bad				( -1 )
#define AUResultCode_OK					( 0 )
#define AUResultCode_NotConnected		( 1 )
#define AUResultCode_BadPlatform		( 2 )
#define AUResultCode_UpdateAvailable	( 3 )
#define AUResultCode_SocketError		( 4 )
#define AUResultCode_InvalidPacket		( 5 )
#define AUResultCode_InvalidPacketType	( 6 )

#define AUPacketHeader { 0xFF, 0xFF, 0x41, 0x75, 0x74, 0x6F, 0x55, 0x70, 0x64, 0x61, 0x74, 0x65 } // bFF bFF A u t o U p d a t e

constexpr unsigned char __AUPacketHeader[] = AUPacketHeader;
constexpr unsigned int AUPacketHeaderSize = sizeof(__AUPacketHeader) / sizeof(__AUPacketHeader[0]);

enum AUPacketType
{
	AU_PACKET_ESTABLISH_CONNECTION = 0, // [ int unused (0x4) ] -> [ int code (0x4) ]
	AU_PACKET_DISCONNECT, // [ int code (0x4) ] -> [ void ]

	AU_PACKET_RESPONSE, // [ int code (0x4) ] -> [ void ]

	AU_PACKET_PLATFORM, // [ int platform (0x4) ] -> [ int code (0x4) ]
	AU_PACKET_APP_VERSION, // [ app_version_t (sizeof app_version_t) ] -> [ int code (0x4) ]

	AU_PACKET_QUERY_PROTOCOL_VERSION, // [ int version (0x4) ] -> ? <<<|>>> not used
	AU_PACKET_QUERY_UPDATE, // [ void ] -> [ int size ], [ uint8_t encrypt_key[FLEXIBLE_SIZE], uint8_t app[FLEXIBLE_SIZE] ]

	AU_PACKET_LAST = AU_PACKET_QUERY_UPDATE
};

struct AUHeader
{
	unsigned char sequence[AUPacketHeaderSize];
};

struct AUPacket
{
	AUHeader header;
	AUPacketType type;
	unsigned int length; // length of further data
};

bool AU_Protocol_InitPacket(struct AUPacket *packet, int type, int size);
bool AU_Protocol_PacketIsValid(struct AUPacket *packet);

#endif