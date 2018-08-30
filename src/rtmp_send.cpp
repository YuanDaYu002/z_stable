#include <stdlib.h> 
#include <stdio.h>
#include <string.h>
#include <sys/vfs.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
//#include <json.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h> 
#include <sys/stat.h>  

#include "rtmp_send.h"
#include "h264parser.h"

bool SendMetadata2(metavideo_t *lpMetaData);
// NALU单元
typedef struct _NaluUnit
{
	int type;
	int size;
	char *data;
}NaluUnit;

RTMP*		m_pRtmp;

char * put_byte( char *output, uint8_t nVal )    
{    
    output[0] = nVal;    
    return output+1;    
}    
char * put_be16(char *output, uint16_t nVal )    
{    
    output[1] = nVal & 0xff;    
    output[0] = nVal >> 8;    
    return output+2;    
}    
char * put_be24(char *output,uint32_t nVal )    
{    
    output[2] = nVal & 0xff;    
    output[1] = nVal >> 8;    
    output[0] = nVal >> 16;    
    return output+3;    
}    
char * put_be32(char *output, uint32_t nVal )    
{    
    output[3] = nVal & 0xff;    
    output[2] = nVal >> 8;    
    output[1] = nVal >> 16;    
    output[0] = nVal >> 24;    
    return output+4;    
}    
char *  put_be64( char *output, uint64_t nVal )    
{    
    output=put_be32( output, nVal >> 32 );    
    output=put_be32( output, nVal );    
    return output;    
}    
char * put_amf_string( char *c, const char *str )    
{    
    uint16_t len = strlen( str );    
    c=put_be16( c, len );    
    memcpy(c,str,len);    
    return c+len;    
}    
char * put_amf_double( char *c, double d )    
{    
    *c++ = AMF_NUMBER;  /* type: Number */    
    {    
        unsigned char *ci, *co;    
        ci = (unsigned char *)&d;    
        co = (unsigned char *)c;    
        co[0] = ci[7];    
        co[1] = ci[6];    
        co[2] = ci[5];    
        co[3] = ci[4];    
        co[4] = ci[3];    
        co[5] = ci[2];    
        co[6] = ci[1];    
        co[7] = ci[0];    
    }    
    return c+8;    
}  
 char * put_amf_boolean( char *c,int v)
 {
   *c++ = AMF_BOOLEAN;	/* type: Number */
   {
	 unsigned char   *co;
	 co = (unsigned char *)c;
	 co[0] = v;
   }
   return c+1;
 }

 long long GetTimeStampMS()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec)*1000 + (tv.tv_usec)/1000;
}

rtmp_t * rtmp_connect(const char* url)  
{  

	printf("start rtmp_connect:%s\n",url);
	rtmp_t *rtmp_ctx = (rtmp_t*)calloc(sizeof(rtmp_t),1);	

	rtmp_ctx->m_pRtmp = RTMP_Alloc();
	RTMP_Init(rtmp_ctx->m_pRtmp);
	/*设置URL*/
	if (RTMP_SetupURL(rtmp_ctx->m_pRtmp,(char*)url) == FALSE)
	{
		RTMP_Free(rtmp_ctx->m_pRtmp);
		return NULL;
	}
	/*设置可写,即发布流,这个函数必须在连接前使用,否则无效*/
	RTMP_EnableWrite(rtmp_ctx->m_pRtmp);
	/*连接服务器*/
	if (RTMP_Connect(rtmp_ctx->m_pRtmp, NULL) == FALSE) 
	{
		RTMP_Free(rtmp_ctx->m_pRtmp);
		return NULL;
	} 

	/*连接流*/
	if (RTMP_ConnectStream(rtmp_ctx->m_pRtmp,0) == FALSE)
	{
		RTMP_Close(rtmp_ctx->m_pRtmp);
		RTMP_Free(rtmp_ctx->m_pRtmp);
		return NULL;
	}
	//rtmp_ctx->videots= (unsigned int)GetTimeStampMS();
	//rtmp_ctx->audiots= (unsigned int)rtmp_ctx->videots;
	printf("==================>ts:%d\n",rtmp_ctx->videots);
	m_pRtmp = rtmp_ctx->m_pRtmp;
	return rtmp_ctx;  
} 
#if 1	
int rtmp_make_video_meta(rtmp_t *ctx)
{
	SendMetadata2(&ctx->m_video);
	return TRUE;
	
	RTMPPacket * packet=NULL;//rtmp包结构
	unsigned char * body=NULL;
	//metavideo_t *meta = &(ctx->m_video);
	//int i;
	int nRet =FALSE;
	packet = (RTMPPacket *)calloc(RTMP_HEAD_SIZE+1024,1);
	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	body = (unsigned char *)packet->m_body;


	char buf[1024] = {0}; 

	char *p = (char *)buf;	  
	p = put_byte(p, AMF_STRING );  
	p = put_amf_string(p , "@setDataFrame" );  

	p = put_byte( p, AMF_STRING );  
	p = put_amf_string( p, "onMetaData" );  

	p = put_byte(p, AMF_OBJECT );    
	p = put_amf_string( p, "copyright" );    
	p = put_byte(p, AMF_STRING );    
	p = put_amf_string( p, "firehood" );	  

	p =put_amf_string( p, "width");  
	p =put_amf_double( p, ctx->m_video.nWidth);  

	p =put_amf_string( p, "height");	
	p =put_amf_double( p, ctx->m_video.nHeight);  

	p =put_amf_string( p, "framerate" );	
	p =put_amf_double( p, 20);   

	p =put_amf_string( p, "videocodecid" );  
	p =put_amf_double( p, 7 );  

#if 0
	p =put_amf_string( p, "audiocodecid" );	
	p =put_amf_double( p, 7);   

	p =put_amf_string( p, "audiosamplerate" );	
	p =put_amf_double( p, 8000);  


	p =put_amf_string( p, "audiosamplesize" );  
	p =put_amf_double( p, 160 );  


#endif
	

	p =put_amf_string( p, "" );  
	p =put_byte( p, AMF_OBJECT_END  );  
	int len = p-buf;  

	memcpy(body,buf,len);
	packet->m_packetType = RTMP_PACKET_TYPE_INFO;
	packet->m_nBodySize = len;
	packet->m_nChannel = 0x04;
	packet->m_nTimeStamp = ctx->videots;
	packet->m_hasAbsTimestamp = 0;
	packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
	packet->m_nInfoField2 = ctx->m_pRtmp->m_stream_id;
	nRet = RTMP_SendPacket(ctx->m_pRtmp,packet,TRUE);
	printf("----------------->send video meta data :%d,%d !!\n",nRet,len);
	free(packet);	 //释放内存
	return nRet;
}
int rtmp_make_video_spspps(rtmp_t *ctx)
{
	RTMPPacket * packet=NULL;//rtmp包结构
	unsigned char * body=NULL;
	metavideo_t *meta = &(ctx->m_video);
	int i;
	int nRet =FALSE;
	packet = (RTMPPacket *)calloc(RTMP_HEAD_SIZE+1024,1);
	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	body = (unsigned char *)packet->m_body;


	i = 0;
	body[i++] = 0x17;
	body[i++] = 0x00;

	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;

	/*AVCDecoderConfigurationRecord*/
	body[i++] = 0x01;
	body[i++] = meta->Sps[1];
	body[i++] = meta->Sps[2];
	body[i++] = meta->Sps[3];
	body[i++] = 0xff;
	/*sps*/
	body[i++]	= 0xe1;
	body[i++] = (meta->nSpsLen >> 8) & 0xff;
	body[i++] = meta->nSpsLen & 0xff;
	memcpy(&body[i],meta->Sps,meta->nSpsLen);
	i +=  meta->nSpsLen;

	/*pps*/
	body[i++]	= 0x01;
	body[i++] = (meta->nPpsLen>> 8) & 0xff;
	body[i++] = (meta->nPpsLen) & 0xff;
	memcpy(&body[i],meta->Pps,meta->nPpsLen);
	i +=  meta->nPpsLen;

	packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
	packet->m_nBodySize = i;
	packet->m_nChannel = 0x04;
	packet->m_nTimeStamp = ctx->videots;
	packet->m_hasAbsTimestamp = 0;
	packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
	packet->m_nInfoField2 = ctx->m_pRtmp->m_stream_id;
	//printf("----------------->send video_spspps :%d,%d !!\n",nRet,i);

	/*调用发送接口*/
	nRet = RTMP_SendPacket(ctx->m_pRtmp,packet,TRUE);
	free(packet);	 //释放内存
	return nRet;
}


#else
int rtmp_make_video_meta(rtmp_t *ctx)
{
	RTMPPacket * packet=NULL;//rtmp包结构
	unsigned char * body=NULL;
	metavideo_t *meta = &(ctx->m_video);
	int i;
	//packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE+1024);
	packet = (RTMPPacket *)calloc(RTMP_HEAD_SIZE+1024,1);
	//RTMPPacket_Reset(packet);//重置packet状态
	memset(packet,0,RTMP_HEAD_SIZE+1024);
	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	body = (unsigned char *)packet->m_body;
	i = 0;
	body[i++] = 0x17;
	body[i++] = 0x00;

	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;

	/*AVCDecoderConfigurationRecord*/
	body[i++] = 0x01;
	body[i++] = meta->Sps[1];
	body[i++] = meta->Sps[2];
	body[i++] = meta->Sps[3];
	body[i++] = 0xff;

	/*sps*/
	body[i++]	= 0xe1;
	body[i++] = (meta->nSpsLen >> 8) & 0xff;
	body[i++] = meta->nSpsLen & 0xff;
	memcpy(&body[i],meta->Sps,meta->nSpsLen);
	i +=  meta->nSpsLen;

	/*pps*/
	body[i++]	= 0x01;
	body[i++] = (meta->nPpsLen>> 8) & 0xff;
	body[i++] = (meta->nPpsLen) & 0xff;
	memcpy(&body[i],meta->Pps,meta->nPpsLen);
	i +=  meta->nPpsLen;

	packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
	packet->m_nBodySize = i;
	packet->m_nChannel = 0x04;
	packet->m_nTimeStamp = 0;
	packet->m_hasAbsTimestamp = 0;
	packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	packet->m_nInfoField2 = ctx->m_pRtmp->m_stream_id;

	/*调用发送接口*/
	int nRet = RTMP_SendPacket(ctx->m_pRtmp,packet,TRUE);
	free(packet);	 //释放内存
	return nRet;
}

#endif
int rtmp_make_video_raw(rtmp_t *ctx,unsigned int nPacketType,unsigned char *data,unsigned int size,char flag,unsigned int nTimeStamp)
{  
	RTMPPacket* packet;
	int bodysize =size+9;
	/*分配包内存和初始化,len为包体长度*/
	packet = (RTMPPacket *)calloc(RTMP_HEAD_SIZE+bodysize,1);
	
	/*包体内存*/
	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	
	packet->m_nBodySize = bodysize;
	char *body = ( char *)packet->m_body;
	int i = 0; 
	body[i++] = flag;
	body[i++] = 0x01;// AVC NALU   
	body[i++] = 0x00;  
	body[i++] = 0x00;  
	body[i++] = 0x00;
	body[i++] = size>>24 &0xff;  
	body[i++] = size>>16 &0xff;  
	body[i++] = size>>8 &0xff;  
	body[i++] = size&0xff;
	memcpy(&body[i],data,size); 	
	packet->m_hasAbsTimestamp = 0;
	packet->m_packetType = nPacketType; /*此处为类型有两种一种是音频,一种是视频*/
	packet->m_nInfoField2 = ctx->m_pRtmp->m_stream_id;
	packet->m_nChannel = 0x04;

	packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
	if (RTMP_PACKET_TYPE_AUDIO ==nPacketType && size !=4)
	{
		packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	}
	packet->m_nTimeStamp = nTimeStamp;
	/*发送*/
	int nRet =FALSE;
	if (RTMP_IsConnected(ctx->m_pRtmp))
	{
		nRet = RTMP_SendPacket(ctx->m_pRtmp,packet,TRUE); /*TRUE为放进发送队列,FALSE是不放进发送队列,直接发送*/
	}
	/*释放内存*/
	free(packet);
	return nRet;  
}
//rtmp://11-rtsmserver.myzmodo.com:443/live/V200V200V200V20_0_0?1554467244

int rtmp_make_audio_spec(rtmp_t *ctx,unsigned int nTimeStamp)
{
	RTMPPacket * packet=NULL;//rtmp包结构
	unsigned char * body=NULL;
	int i;
	packet = (RTMPPacket *)calloc(RTMP_HEAD_SIZE+4,1);
	//packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE+4);
	memset(packet,0,RTMP_HEAD_SIZE);
	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	body = (unsigned char *)packet->m_body;
	i = 0;
	//低4位的前2位代表抽样频率，二进制11代表44kHZ。第3位代表 音频用16位的。第4个bit代表声道数
	//body[i++] = 0x72; //   0111,0010
	//body[i++] = 0x00;
	#if 0
	body[i++] = 0xAF; //  aac
	body[i++] = 0x00;
	#else
	body[i++] = 0x72;
	body[i++] = 0x00;
	#endif
	packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
	packet->m_nBodySize = 4;
	packet->m_nChannel = 0x04;
	packet->m_nTimeStamp = nTimeStamp;
	packet->m_hasAbsTimestamp = 0;
	packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	packet->m_nInfoField2 = ctx->m_pRtmp->m_stream_id;
	//if (RTMP_PACKET_TYPE_AUDIO ==packet->m_packetType && packet->m_nBodySize !=4)
	//{
	//	packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	//}
	
	/*调用发送接口*/
	int nRet = RTMP_SendPacket(ctx->m_pRtmp,packet,TRUE);
	
	//printf("----------------->send audio_spec data :%d!!\n",nRet);
	free(packet);	 //释放内存
	return nRet;
}

int rtmp_make_audio_raw(rtmp_t *ctx,unsigned char *data,unsigned int size,unsigned int nTimeStamp)
{  
	RTMPPacket* packet;

	/*分配包内存和初始化,len为包体长度*/
	packet = (RTMPPacket *)calloc(RTMP_HEAD_SIZE+size+1,1);
	memset(packet,0,RTMP_HEAD_SIZE+size+1);
	/*包体内存*/
	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	

	unsigned char *body = (unsigned char *)packet->m_body;
	memset(body, 0, size+1);
	#if 1
	body[0] = 0x72;
	//body[1] = 0x01;	
	#else
	body[0] = 0xAF;
	body[1] = 0x01;	
	#endif
	memcpy(&body[1], data, size);
	packet->m_nBodySize = size+1;
	packet->m_hasAbsTimestamp = 0;
	packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
	packet->m_nInfoField2 = ctx->m_pRtmp->m_stream_id;
	packet->m_nChannel = 0x04;
	packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;

	packet->m_nTimeStamp = nTimeStamp;
	/*发送*/
	int nRet =FALSE;
	if (RTMP_IsConnected(ctx->m_pRtmp))
	{
		nRet = RTMP_SendPacket(ctx->m_pRtmp,packet,TRUE); /*TRUE为放进发送队列,FALSE是不放进发送队列,直接发送*/
	}
	//printf("a:%d\n",nTimeStamp);
	/*释放内存*/
	free(packet);
	return nRet;  
}

int rtmp_video_send(rtmp_t *ctx,unsigned char *data,unsigned int size,int type,unsigned int nTimeStamp)  
{  
	if(data == NULL ){  
		return -1;  
	}
	int nRet =FALSE;
	//printf("rtmp-------------->%d,%04d,%08d\n",type,size,nTimeStamp);
	int flag=0;
	if(type ==RTMP_IFRAME )
	{ 
		flag = 0x17;// 1:Iframe  7:AVC  
		if(rtmp_make_video_spspps(ctx)==FALSE)
			return FALSE;

	}
	else
	{  
		flag = 0x27;// 2:Pframe  7:AVC   
		//printf("rtmp-------------->%x,%04d,%08d\n",data[0],size,nTimeStamp);
	} 
	//printf("v:%d\n",ctx->videots);
	nRet=rtmp_make_video_raw(ctx,RTMP_PACKET_TYPE_VIDEO,data,size,flag,nTimeStamp);
	return nRet;
} 


int SendPacket(unsigned int nPacketType,unsigned char *data,unsigned int size,unsigned int nTimestamp)
{
	if(m_pRtmp == NULL)
	{
		return FALSE;
	}

	RTMPPacket packet;
	RTMPPacket_Reset(&packet);
	RTMPPacket_Alloc(&packet,size);

	packet.m_packetType = nPacketType;
	packet.m_nChannel = 0x04;  
	packet.m_headerType = RTMP_PACKET_SIZE_LARGE;  
	packet.m_nTimeStamp = nTimestamp;  
	packet.m_nInfoField2 = m_pRtmp->m_stream_id;
	packet.m_nBodySize = size;
	memcpy(packet.m_body,data,size);

	int nRet = RTMP_SendPacket(m_pRtmp,&packet,TRUE);
	//printf("SendPacket-------------->nRet:%d,size:%d\n",nRet,size);

	RTMPPacket_Free(&packet);

	return nRet;
}

bool SendMetadata(metavideo_t *lpMetaData)
{
	if(lpMetaData == NULL)
	{
		return false;
	}
	int Ret = FALSE;
	char body[1024] = {0};;
    
    char * p = (char *)body;  
	p = put_byte(p, AMF_STRING );
	p = put_amf_string(p , "@setDataFrame" );

	p = put_byte( p, AMF_STRING );
	p = put_amf_string( p, "onMetaData" );

	p = put_byte(p, AMF_OBJECT );  
	p = put_amf_string( p, "copyright" ); 
	
	p = put_byte(p, AMF_STRING );  
	p = put_amf_string( p, "firehood" );  

	p =put_amf_string( p, "width");
	p =put_amf_double( p, lpMetaData->nWidth);

	p =put_amf_string( p, "height");
	p =put_amf_double( p, lpMetaData->nHeight);

	p =put_amf_string( p, "framerate" );
	p =put_amf_double( p, lpMetaData->nFrameRate); 

	p =put_amf_string( p, "videocodecid" );
	p =put_amf_double( p, 7 );

#if 0

	p =put_amf_string( p, "audiocodecid" );
	p =put_amf_double( p, 7); 




	p =put_amf_string( p, "audiosamplerate" );
	p =put_amf_double( p, 8000); 



	p =put_amf_string( p, "audiosamplesize" );
	p =put_amf_double( p, 160); 

	

#endif
	




	

	p =put_amf_string( p, "" );
	p =put_byte( p, AMF_OBJECT_END  );
	int i = 0;
	int index = p-body;


	Ret=SendPacket(RTMP_PACKET_TYPE_INFO,(unsigned char*)body,p-body,0);
#if 0

	i=0;
	//低4位的前2位代表抽样频率，二进制11代表44kHZ。第3位代表 音频用16位的。第4个bit代表声道数
	body[i++] = 0xAF; //   0111,0010
	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;
	


	Ret = SendPacket(RTMP_PACKET_TYPE_AUDIO,(unsigned char *)body,i,0);
#endif

	printf("SendMetadata-------------->index:%d,framerate:%d,width:%d,width:%d,Ret:%d\n",index,lpMetaData->nFrameRate,lpMetaData->nWidth,lpMetaData->nHeight,Ret);
	i=0;
	body[i++] = 0x17; // 1:keyframe  7:AVC
	body[i++] = 0x00; // AVC sequence header

	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00; // fill in 0;

	// AVCDecoderConfigurationRecord.
	body[i++] = 0x01; // configurationVersion
	body[i++] = lpMetaData->Sps[1]; // AVCProfileIndication
	body[i++] = lpMetaData->Sps[2]; // profile_compatibility
	body[i++] = lpMetaData->Sps[3]; // AVCLevelIndication 
    body[i++] = 0xff; // lengthSizeMinusOne  

    // sps nums
	body[i++] = 0xE1; //&0x1f
	// sps data length
	body[i++] = lpMetaData->nSpsLen>>8;
	body[i++] = lpMetaData->nSpsLen&0xff;
	// sps data
	memcpy(&body[i],lpMetaData->Sps,lpMetaData->nSpsLen);
	i= i+lpMetaData->nSpsLen;

	// pps nums
	body[i++] = 0x01; //&0x1f
	// pps data length 
	body[i++] = lpMetaData->nPpsLen>>8;
	body[i++] = lpMetaData->nPpsLen&0xff;
	// sps data
	memcpy(&body[i],lpMetaData->Pps,lpMetaData->nPpsLen);
	i= i+lpMetaData->nPpsLen;

	return SendPacket(RTMP_PACKET_TYPE_VIDEO,(unsigned char*)body,i,0);

}

bool SendH264Packet(unsigned char *data,unsigned int size,bool bIsKeyFrame,unsigned int nTimeStamp)
{
	if(data == NULL && size<11)
	{
		return false;
	}

	unsigned char *body = new unsigned char[size+9];

	int i = 0;
	if(bIsKeyFrame)
	{
		body[i++] = 0x17;// 1:Iframe  7:AVC
	}
	else
	{
		body[i++] = 0x27;// 2:Pframe  7:AVC
	}
	body[i++] = 0x01;// AVC NALU
	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;

	// NALU size
	body[i++] = size>>24;
	body[i++] = size>>16;
	body[i++] = size>>8;
	body[i++] = size&0xff;;

	// NALU data
	memcpy(&body[i],data,size);

	bool bRet = SendPacket(RTMP_PACKET_TYPE_VIDEO,body,i+size,nTimeStamp);

	delete[] body;

	return bRet;
}

bool ReadOneNaluFromBuf(int m_nFileBufSize,char *m_pFileBuf,int *m_nCurPos,NaluUnit *nalu)
{
	int i = *m_nCurPos;
	while(i<m_nFileBufSize)
	{
		if(m_pFileBuf[i++] == 0x00 &&
			m_pFileBuf[i++] == 0x00 &&
			m_pFileBuf[i++] == 0x00 &&
			m_pFileBuf[i++] == 0x01
			)
		{
			int pos = i;
			while (pos<m_nFileBufSize)
			{
				if(m_pFileBuf[pos++] == 0x00 &&
					m_pFileBuf[pos++] == 0x00 &&
					m_pFileBuf[pos++] == 0x00 &&
					m_pFileBuf[pos++] == 0x01
					)
				{
					break;
				}
			}
			if(pos == m_nFileBufSize)
			{
				nalu->size = pos-i;	
			}
			else
			{
				nalu->size = (pos-4)-i;
			}
			nalu->type = m_pFileBuf[i]&0x1f;
			nalu->data = &m_pFileBuf[i];

			*m_nCurPos = pos-4;
			return TRUE;
		}
	}
	return FALSE;
}
bool SendMetadata2(metavideo_t *lpMetaData)
{
  if(lpMetaData == NULL)
  {
    return false;
  }
  char body[1024] = {0};

  char * p = (char *)body;
  p = put_byte(p, AMF_STRING );
  p = put_amf_string(p , "@setDataFrame" );

  p = put_byte( p, AMF_STRING );
  p = put_amf_string( p, "onMetaData" );

  p = put_byte(p, AMF_ECMA_ARRAY);
  p = put_be32(p, 14);

  p =put_amf_string( p, "duration" );
  p =put_amf_double( p, 0 );

  p =put_amf_string( p, "filesize" );
  p =put_amf_double( p, 0 );

  p =put_amf_string( p, "width");
  p =put_amf_double( p, lpMetaData->nWidth);

  p =put_amf_string( p, "height");
  p =put_amf_double( p, lpMetaData->nHeight);

  p =put_amf_string( p, "videocodecid" );
  p = put_byte(p, AMF_STRING );
  p =put_amf_string( p, "avc1" );

  p =put_amf_string( p, "videodatarate" );
  p =put_amf_double( p, lpMetaData->nFrameRate); 

  p =put_amf_string( p, "audiocodecid" );
  p = put_byte( p, AMF_STRING );
  p =put_amf_string( p, "g711a" );

  

  p =put_amf_string( p, "audiodatarate" );
  p =put_amf_double( p, 160);

  p =put_amf_string( p, "audiosamplerate" );
  p =put_amf_double( p, 8000 );

  p =put_amf_string( p, "audiosamplesize" );
  p =put_amf_double( p, 16 );

  p =put_amf_string( p, "audiochannels" );
  p =put_amf_double( p, 2 );

  p =put_amf_string( p, "stereo" );
  p =put_amf_boolean( p,1);

	p =put_amf_string( p, "2.1" );
	p =put_amf_boolean( p,0);
	p =put_amf_string( p, "3.1" );
	p =put_amf_boolean( p,0);
	p =put_amf_string( p, "4.0" );
	p =put_amf_boolean( p,0);
	p =put_amf_string( p, "4.1" );
	p =put_amf_boolean( p,0);
	p =put_amf_string( p, "5.1" );
	p =put_amf_boolean( p,0);
	p =put_amf_string( p, "7.1" );
	p =put_amf_boolean( p,0);

  p =put_amf_string( p, "" );
  p =put_byte( p, AMF_OBJECT_END  );

  //int index = p-body;

  return SendPacket(RTMP_PACKET_TYPE_INFO,(unsigned char*)body,p-body,0);}

bool SendAudioPacket(unsigned char *data,unsigned int size,unsigned int nTimeStamp)
{	
	#if 0
	
	#else
	if(data == NULL && size<11)
	{
		return FALSE;
	}

	unsigned char *body = new unsigned char[size+4];

	int i = 0;
	//低4位的前2位代表抽样频率，二进制11代表44kHZ。第3位代表 音频用16位的。第4个bit代表声道数
	body[i++] = 0xAF; //   0111,0010
	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;
	

	bool bRet = 0;
	bRet =SendPacket(RTMP_PACKET_TYPE_AUDIO,body,i,nTimeStamp);


	i = 0;
	body[i++] = 0xAF;
	body[i++] = 0x01;	



	memcpy(&body[i],data,size);
	
	bRet = SendPacket(RTMP_PACKET_TYPE_AUDIO,body,i+size,nTimeStamp);
	delete[] body;

	return bRet;
	#endif
}

int rtmp_stream_send(rtmp_t *ctx,unsigned char *data,unsigned int size,unsigned char type )
{

#if 1
	int ret = FALSE;
	if (!RTMP_IsConnected(ctx->m_pRtmp))
	{
		printf("RTMP_IsConnected  Fail!\n");
		return FALSE;
	}
	
	if(type==RTMP_IFRAME)
	{
		ctx->vcount++;
		if(!ctx->m_video.isparser)
		{
			parser_video_meta(data,size,&ctx->m_video);	
			//if(rtmp_make_video_meta(ctx)==FALSE)
			//	return FALSE;
			//if(rtmp_make_video_spspps(ctx)==FALSE)
				//return FALSE;

			if(rtmp_make_audio_spec(ctx,ctx->audiots)==FALSE)
				return FALSE;	
			//ret=rtmp_video_send(ctx,data+ctx->m_video.idr_offset,size-ctx->m_video.idr_offset,type,ctx->videots);
		}
		
		ret=rtmp_video_send(ctx,data+ctx->m_video.idr_offset,size-ctx->m_video.idr_offset,type,ctx->videots);
		//ctx->videots+=100;
		printf("rtmp I frame size:%04d,[count:%d,%d][t:%d,%d]\n",size,ctx->vcount,ctx->acount,ctx->videots,ctx->audiots);
	}
	else if(type==RTMP_PFRAME)
	{	
		unsigned char *p = data+4;
		
		if( ((*p)&0x1f) == 0x01 )
		{
			//printf("direct----------------->p:%x\n",p[0]);
			ret=rtmp_video_send(ctx,data+4,size-4,type,ctx->videots);
		}
		else
		{
			NaluUnit nalu;
			int m_nCurPos=0;
			while(ReadOneNaluFromBuf(size,(char*)data,&m_nCurPos,&nalu))
			{
				//printf("p--->nalu:%x\n",nalu.type);
				if(nalu.type == 0x01)
				{
					//printf("send p frame:%x,size:%d\n",nalu.type,nalu.size);
					ret=rtmp_video_send(ctx,(unsigned char *)nalu.data,nalu.size,type,ctx->videots);
				}					
			
			}
		}		
		//ctx->videots+=100;
		
		ctx->vcount++;
	}
	else if(type==RTMP_AFRAME)
	{	
		
		//if(rtmp_make_audio_spec(ctx,ctx->audiots)==FALSE)
		//	return FALSE;
		if(rtmp_make_audio_raw(ctx,data+4,size-4,ctx->audiots)==FALSE)
			return FALSE;
		//ctx->audiots+=21;
		ctx->acount++;
		
		ret=TRUE;
	}
	return ret;
#else
	int ret = FALSE;
	if (!RTMP_IsConnected(ctx->m_pRtmp))
	{
		printf("RTMP_IsConnected  Fail!\n");
		return FALSE;
	}

	if(type==RTMP_IFRAME)
	{
		if(!ctx->m_video.isparser)
		{
			parser_video_meta(data,size,&ctx->m_video); 
			if(SendMetadata(&ctx->m_video)==FALSE)
				return FALSE;
		}
		//ret=rtmp_video_send(ctx,data+4,size-4,type,ctx->videots);
		//ret=rtmp_video_send(ctx,data+ctx->m_video.idr_offset,size-ctx->m_video.idr_offset,type,ctx->videots);
		NaluUnit nalu;
		int m_nCurPos=0;
		while(ReadOneNaluFromBuf(size,(char*)data,&m_nCurPos,&nalu))
		{
			if(nalu.type == 0x06)continue;
			int  bKeyframe	= (nalu.type == 0x05) ? TRUE : FALSE;
			// 发送H264数据帧
			ret = SendH264Packet((unsigned char *)nalu.data,nalu.size,bKeyframe,ctx->videots);
			//ret = SendH264Packet((unsigned char *)nalu.data,nalu.size,TRUE,ctx->videots);
			//printf("nalu-------------->%d,%d\n",nalu.type,nalu.size);
			ctx->videots+=100;
			printf("rtmp I frame-------------->%d,%04d,%08d,%x,%x\n",type,size,ctx->videots,nalu.data[0],nalu.type);
		}
		
		
	}
	else if(type==RTMP_PFRAME)
	{	
		ret=TRUE;
		//ret=rtmp_video_send(ctx,data+4,size-4,type,ctx->videots);
		ctx->videots+=100;
	}
	else if(type==RTMP_AFRAME)
	{	

		ret=SendAudioPacket(data+4,size-4,ctx->audiots);
		ctx->audiots+=50;
	}

	return ret;
#endif

}
void rtmp_close(rtmp_t *ctx)
{
	if(!ctx)return;
	if(ctx->m_pRtmp)
	{
		RTMP_Close(ctx->m_pRtmp); 
		RTMP_Free(ctx->m_pRtmp);
		ctx->m_pRtmp=NULL;
	}
	
	if(ctx->m_video.Pps)free(ctx->m_video.Pps);
	if(ctx->m_video.Sps)free(ctx->m_video.Sps);
	ctx->m_video.isparser=0;
	ctx->m_video.idr_offset=0;
	free(ctx);
	ctx=NULL;
	
		
}

