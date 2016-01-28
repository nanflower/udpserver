#ifndef UDP_H
#define UDP_H

#include "udpsocket.h"
#include "one_process.h"
#include "decodepool.h"
#define Channel_NUM 14

class udp
{
public:
    udp();
    ~udp();
    int Init();

private:
    udpsocket* m_ChannelGet[2];
    one_process* m_VideoEncode[Channel_NUM];

};



#endif // UDP_H
