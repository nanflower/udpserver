#ifndef TRANSCODEPOOL_H
#define TRANSCODEPOOL_H


class transcodepool
{
public:
    transcodepool();
    ~transcodepool();
    void Init();
    bool GetFrame( void **ppFrameBuf, unsigned long* plLength, unsigned long * plTimeStamp );
    bool PutFrame( void **ppFrameBuf, unsigned long* plLength, unsigned long * plTimeStamp );
};

#endif // TRANSCODEPOOL_H
