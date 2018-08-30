#ifndef _BUFFER_MANAGE_H_
#define _BUFFER_MANAGE_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include "common.h"

//#define TEST_ENCODE_DATA  
//#define TEST_AENCODE_DATA  
#define MAX_FRM_NUM			(30*150)
#define MAX_V_FRM_NUM			(30*30)
#define MAX_I_F_NUM			30

//最大预录时长
#define MAX_PRECORD_SEC		30//20//16
#define MAX_ARECORD_SEC		90

//D1 + AUDIO buffer 大小
#define BUFFER_SIZE_D1_AUDIO	0x400000

//D1 buffer 大小
#define BUFFER_SIZE_D1			0x400000

//CIF buffer 大小
#define BUFFER_SIZE_CIF			0x400000

#define	DEFAULT_IFRAMEOFFSET				0xffff

/*门铃1代不丢帧*/
#ifndef DOOR_BELL
#define ADJUST_STREAM
#endif

#ifdef  ADJUST_STREAM
/*
IframeGop		I帧间隔			---------单位s
FrameDiffPos		读写指针间隔 ---------单位帧

*/
#define		IframeGop 			5
#define		FrameDiffPos		6
#endif
//帧类型
typedef enum{
	I_FRAME = 1,
	P_FRAME,
	A_FRAME,
	B_FRAME
}FrameType_E;

//系统日期时间结构体定义
typedef struct{
	char		 year;
	char		 month;
	char		 mday;
	char		 hour;
	char		 minute;
	char		 second;
	char		 week;		
	char		 reserve;
}SystemDateTime, *pSystemDateTime;

// 用于获取H.264 数据NAL单元信息
#define MAX_PKT_NUM_IN_STREAM 10
typedef enum nalu_type { 
    NALU_PSLICE = 1, 
    NALU_ISLICE = 5, 
    NALU_SEI = 6, 
    NALU_SPS = 7, 
    NALU_PPS = 8, 
} nalu_type_t;

typedef struct venc_pkt {
    unsigned len;
    nalu_type_t type;
} venc_pkt_t;

typedef struct venc_stream {
    venc_pkt_t pkt[MAX_PKT_NUM_IN_STREAM];
    unsigned pkt_cnt; 
    unsigned seq;
} venc_stream_t;
//一个视频帧或音频帧在缓冲池的信息结构体
typedef struct {
	unsigned long			FrmStartPos;/*此帧在buffer中的偏移*/
	unsigned long			FrmLength;  /*此帧的有效数据长度*/
	long long				Pts;			/*如果是视频帧，则为此帧的时间戳*/
	unsigned char			Flag;		/* 1 I 帧, 2 P 帧, 3 音频帧*/
	unsigned char			hour;		/*产生此帧的时间*/
	unsigned char			min;
	unsigned char			sec;
	venc_stream_t			venc_stream; /* 可能包含多个NAL单元 */
    bool                    talk;
}FrameInfo, *pFrameInfo;

typedef struct 
{
    unsigned int    m_nVHeaderFlag; // 帧标识，00dc, 01dc, 01wb
    unsigned int    m_nVFrameLen;  // 帧的长度
    unsigned char   m_u8Hour;
    unsigned char   m_u8Minute;
    unsigned char   m_u8Sec;
    unsigned char   m_u8Pad0;// 代表附加消息的类型，根据这个类型决定其信息结构0 代表没有1.2.3 各代表其信息
    unsigned int    m_nILastOffset;// 此帧相对上一个I FRAME 的偏移只对Iframe 有用
    long long      	m_lVPts;    // 时间戳
    unsigned int    m_bMdAlarm:1;/*bit0 移动侦测报警1:报警，0:没有报警*/
    unsigned int    m_FrameType:4;/*帧类型*/
    unsigned int    m_Lost:1;
    unsigned int    m_FrameRate:5;

    unsigned int    m_bPirAlarm:1; /*bit11*/ 
    unsigned int    m_bMicroWaveAlarm:1; /*bit12*/ 
    unsigned int    m_b915Alarm:1; /*bit13*/ 
    unsigned int    m_bEncrypt:1; /*bit14*/ 
    unsigned int    m_bPirAlarm2:1; /*bit15 第二个pir告警*/ 
    unsigned int    m_bPirAlarm3:1; /*bit16 第三个pir告警*/   
    unsigned int    m_b3CloudRecord:3; /*bit17~bit19 云存储标记0:未触发云存储1:触发云存储2~7未定义*/   
    unsigned int    m_Res:12; /*bit20-bit31 暂时保留*/ 
    unsigned int    m_nReserved;
}VideoFrameHeader;

typedef struct 
{
	unsigned int		m_nAHeaderFlag; // 帧标识，00dc, 01dc, 01wb
	unsigned int 		m_nAFrameLen;  // 帧的长度,不包括头
	long long			m_lAPts;		// 时间戳
}AudioFrameHeader;

//帧缓冲池的结构定义
typedef struct {
	unsigned char  			*bufferstart;				/*媒体数据buf 起始地址*/
	unsigned long               		buffersize;				/*buf 空间大小*/
	unsigned long	      			writepos;				/*写指针偏移*/
	unsigned long				readpos;				/*读指针的偏移*/

	FrameInfo				FrmList[MAX_FRM_NUM];	/*buf 中存储的帧列表信息*/
	unsigned short		 	CurFrmIndex;			/*帧列表中的下标*/			
	unsigned short		 	TotalFrm;				/*buffer 中的总帧数*/
	
	unsigned short 			IFrmList[MAX_I_F_NUM];	/*最近n 个i 帧在FrmList中的数组下标信息*/
	unsigned short			IFrmIndex;				/*当前I 帧列表索引*/
	unsigned short			TotalIFrm;				/*总的I 帧数目*/
	unsigned short			ICurIndex;				//当前I帧序列
	
	unsigned long				circlenum;				/*buf覆盖的圈数*/

	unsigned long				m_u32MaxWpos;			/*最大写指针位置*/
	
}FrameBufferPool, *pFrameBufferPool;

//一个FrameBufferPool 用户结构体定义
typedef struct{
	unsigned short		ReadFrmIndex;			/*此用户对帧缓冲池访问所用的帧索引*/
	unsigned short		reserve;
	unsigned long		ReadCircleNum;			/*此用户对帧缓冲池的访问圈数，初始时等于
												帧缓冲池中的circlenum*/
	unsigned int		diffpos;				/*读指针和写指针位置差值，单位为帧*/
	unsigned int 		throwframcount;			/*从开始计数丢帧的个数*/
}FrameBufferUser, *pFrameBufferUser;

//日期时间回调函数定义
typedef int (*DateTimeCallBack)(SystemDateTime *pSysTime);

#endif
