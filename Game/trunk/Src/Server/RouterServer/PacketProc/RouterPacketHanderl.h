﻿#ifndef __ROUTERPACKETHANDLER_H__
#define __ROUTERPACKETHANDLER_H__

#include "Server/Base/PacketHandler.h"

class RouterPacketHandler : public PacketHandler
{
public:
	RouterPacketHandler();
	virtual void OnRecvInnerPacket(int nSrcSessionID, Packet* poPacket, INNER_HEADER& oHeader, int* ppSessionArray);

protected:
	void Forward(int nSrcSessionID, Packet* poPacket, INNER_HEADER& oHeader);
	
private:
	DISALLOW_COPY_AND_ASSIGN(RouterPacketHandler);
};

#endif