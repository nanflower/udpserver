#include "udp.h"


udp::udp()
{

}

udp::~udp()
{

}

int udp::Init()
{

        for(int i=0;i<1;i++){
            m_ChannelGet[i] = new udpsocket();
            m_ChannelGet[i]->thread_init(i);  //udp接收线程
       //     printf("thread %d create\n",i);
      //      m_ChannelGet[i].ts_demux();     //ts demux线程
        }

        for(int i=0; i<1; i++)
        {
            m_VideoEncode[i] = new one_process();
            m_VideoEncode[i]->Init(i);
        }

        return 0;
}



