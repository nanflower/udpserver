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
            m_VideoEncode = new one_process();
            m_VideoEncode->Init(i);
       //     printf("thread %d create\n",i);
      //      m_ChannelGet[i].ts_demux();     //ts demux线程
        }

        return 0;
}



