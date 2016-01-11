#ifndef UDP_H
#define UDP_H

#include "udpsocket.h"
#include "one_process.h"


class udp
{
public:
    udp();
    ~udp();
    int Init();
private:
    udpsocket* m_ChannelGet[2];
    one_process* m_VideoEncode;

};



#endif // UDP_H
