#ifndef __RTMP_SEND_H__
#define __RTMP_SEND_H__
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "rtmp.h"   
#include "rtmp_sys.h"   
#include "amf.h"  
#include "h264parser.h"
 //定义包头长度，RTMP_MAX_HEADER_SIZE=18
#define RTMP_HEAD_SIZE   (sizeof(RTMPPacket)+RTMP_MAX_HEADER_SIZE)
//#define RTMP_HEAD_SIZE   (sizeof(RTMPPacket))

//存储Nal单元数据的buffer大小
#define BUFFER_SIZE 32768*100
//搜寻Nal单元时的一些标志
#define GOT_A_NAL_CROSS_BUFFER BUFFER_SIZE+1
#define GOT_A_NAL_INCLUDE_A_BUFFER BUFFER_SIZE+2
#define NO_MORE_BUFFER_TO_READ BUFFER_SIZE+3

typedef struct
{
	RTMP*		m_pRtmp;
	metavideo_t m_video;
	metaaudio_t m_audio;
	int 	videots;
	int 	audiots;
	int 	acount;
	int 	vcount;
} rtmp_t;
typedef enum {
    RTMP_IFRAME = 0,
    RTMP_PFRAME = 1,
    RTMP_AFRAME = 2,
} RTMP_TYPE;

rtmp_t * rtmp_connect(const char* url) ;
int rtmp_stream_send(rtmp_t *ctx,unsigned char *data,unsigned int size,unsigned char type);
void rtmp_close(rtmp_t *ctx);

#endif
