#include "udp.h"


udp::udp()
{

}

udp::~udp()
{

}

int udp::Init()
{

        udpsocket m_ChannelGet[3];

        for(int i=0;i<1;i++){
            m_ChannelGet[i].thread_init(i);
            m_ChannelGet[i].ts_demux();
        }

        return 0;
}



