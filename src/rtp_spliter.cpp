#include "rtp_spliter.h" 
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "zmd_msg.h"
#include "plog.h"
#include "p2pconfig.h"


RTPSpliter::SendBuffer::SendBuffer():m_buffer(NULL), m_bufferSize(0), m_filledSize(0)
{

}
RTPSpliter::SendBuffer::~SendBuffer()
{
    if(m_buffer) free(m_buffer);
}
void RTPSpliter::SendBuffer::SetBufferSize(int size)
{
    if(m_buffer && m_bufferSize < size)
    {
        free(m_buffer);
        m_buffer = (char*)malloc(size);
        m_bufferSize = size;
        plog("resize spliter buffer size to %d\n", size);
    }
    else
    {
        m_buffer = (char*)malloc(size);
        m_bufferSize = size;
    }
    P2P_ASSERT(m_buffer != NULL);
}

char* RTPSpliter::SendBuffer::GetBuffer()
{
    return m_buffer;
}

int RTPSpliter::SendBuffer::GetBufferSize()
{
    return m_bufferSize;
}

int RTPSpliter::SendBuffer::PushBack(const char* buffer, int bsize)
{
    if(m_filledSize + bsize > m_bufferSize)
        bsize = m_bufferSize - m_filledSize;

    memcpy(m_buffer+ m_filledSize, buffer, bsize);
    m_filledSize+=bsize;
    return bsize;
}
int RTPSpliter::SendBuffer::GetFilledSize()
{
    return m_filledSize;
}
void RTPSpliter::SendBuffer::ResetBuffer()
{
    m_filledSize = 0;
}
/***************************************/
/*RTPSpliter                           */
/**************************************/
RTPSpliter::RTPSpliter()
{
    m_buffer.SetBufferSize(20*1024);
}

RTPSpliter::~RTPSpliter()
{

}

void RTPSpliter::SplitFrame(int cmd_type, const char* buffer, int size, int frame_flag, int chlnum)
{
    int bsize_filled = 0;
    int slic_size = 0;
    int slic_num = 0;
    int slic_cnt = size/RTP_SLIC_SIZE + ((size%RTP_SLIC_SIZE)>0?1:0);

    if(size*1.2f> m_buffer.GetBufferSize())
        m_buffer.SetBufferSize(size*1.2f);
    m_buffer.ResetBuffer();

    trans_video_s head = P2P_HEAD_INITIALIZER;
    head.cmd_type = htons(cmd_type);
    head.cmd = htonl(PC_IPC_TRANSFER_DATA);
    head.chlnum = chlnum;
    head.frame_flag = frame_flag;

    while(bsize_filled < size)
    {
        if(size - bsize_filled > RTP_SLIC_SIZE)
        {
            slic_size = RTP_SLIC_SIZE;
        }
        else
        {
            slic_size = size - bsize_filled;
        }
        head.length = htonl(slic_size);
        head.seqnum = slic_cnt;
        head.seqnum |= slic_num++<<16; 
        head.seqnum = htonl(head.seqnum);
        //plog("slicnum:%d, slicnt:%d\n", slic_num, slic_cnt);

        m_buffer.PushBack((char*)&head, P2P_HEAD_LEN);
        m_buffer.PushBack(buffer + bsize_filled, slic_size);
        bsize_filled += slic_size;
    }
}

const char* RTPSpliter::GetSendBuffer()
{
    return m_buffer.GetBuffer();
}
int RTPSpliter::GetSendBufferSize()
{
    return m_buffer.GetFilledSize();
}
