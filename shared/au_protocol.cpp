#pragma warning(disable : 26812)

#include "au_protocol.h"

#include <string.h>

unsigned char g_AUPacketHeader[] = AUPacketHeader;

bool AU_Protocol_InitPacket(struct AUPacket *packet, int type, int length)
{
	memcpy( &packet->header, g_AUPacketHeader, AUPacketHeaderSize );
	
	packet->type = (AUPacketType)type;
	packet->length = length;

	return true;
}

bool AU_Protocol_PacketIsValid(struct AUPacket *packet)
{
	return ( memcmp(&packet->header, g_AUPacketHeader, AUPacketHeaderSize) == 0 ) && ( packet->type >= 0 && packet->type <= AU_PACKET_LAST );
}