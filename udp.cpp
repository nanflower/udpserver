#include "udp.h"

udp::udp()
{

}

udp::~udp()
{

}

void udp::Init()
{

    udpsocket m_ChannelGet[3];

    for(int i=0;i<3;i++)
        m_ChannelGet[i].threadinit(i);

}

