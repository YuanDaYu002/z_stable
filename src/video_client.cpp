#include <stdio.h>
#include "video_client.h"
#include "plog.h"
#include "zmd_msg.h"
#include "device_operation.h"
#include "BufferManage.h"

VideoClient::VideoClient()
{
    m_mediaid = NULL;
    m_audio = false;
    m_frameflag = 0;
    m_chlnum = 0;
}
VideoClient::~VideoClient()
{
    ReleaseMedia();
}
bool VideoClient::RequestMedia(int chlnum, int streamtype)
{
    ReleaseMedia();

    m_chlnum = chlnum;
    m_mediaid = p2p_get_media_id(chlnum,streamtype);
    if(m_mediaid == 0) return false;
    p2p_set_i_frame(m_mediaid);
    return true;
}
void VideoClient::RequestAuido(bool on)
{
    m_audio = on;
}
int  VideoClient::GetOneFrame(void** out_data, int* out_len)
{
    unsigned char* data = 0 ;
    FrameInfo info={0} ;
    int getRet = 0 ;

    do
    {
        getRet = p2p_get_one_frame(m_mediaid, &data , &info );
        if( !getRet && data && info.FrmLength > 0 )
        {
            if( info.FrmLength >= MAX_FRAME_SIZ)
            {
                plog("frame to big!!! (%lu)\n", info.FrmLength);
                return -1;
            }
            if( info.Flag == 3 && !m_audio)
            {
                return 0;
            }			

            int flag = 0;
            VideoFrameHeader* fh = (VideoFrameHeader*)data;;

            if(fh->m_nVHeaderFlag != 0x63643030 && 
                    fh->m_nVHeaderFlag != 0x63643130 &&
                    fh->m_nVHeaderFlag != 0x62773130)
            {
                plog("invalid header flag:[0x%x]\n", fh->m_nVHeaderFlag);
                p2p_set_i_frame(m_mediaid);
                return 0;
            }
            if( info.Flag != 3)
            {
                if(fh->m_FrameType == 0)
                    flag = 0;
                else if(fh->m_FrameType == 1) // 
                    flag = 3;
                else if(fh->m_FrameType == 2)
                    flag = (m_frameflag%2)+1;
                else if(fh->m_FrameType == 4)
                    flag = (m_frameflag++%2)+1;
            }
            else 
                flag = 4;
            m_spliter.SplitFrame(CMD_TYPE_IPC_CLIENT, (char*)data, info.FrmLength, flag, m_chlnum);
            *out_data = (void*)m_spliter.GetSendBuffer();
            *out_len  = m_spliter.GetSendBufferSize();
            return 1;
        }
    }while(getRet == -2); // -2 mean buffermanager want to drop frame
    return 0;
}
void VideoClient::ReleaseMedia()
{
    if(m_mediaid)
    {
        p2p_free_media_id(m_mediaid);
        m_mediaid = NULL;
    }
}

