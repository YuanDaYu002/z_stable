#ifndef __H264PARSER_H__
#define __H264PARSER_H__
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <math.h>
#include "plog.h"

typedef enum {
    NALU_TYPE_SLICE = 1,
    NALU_TYPE_DPA = 2,
    NALU_TYPE_DPB = 3,
    NALU_TYPE_DPC = 4,
    NALU_TYPE_IDR = 5,
    NALU_TYPE_SEI = 6,
    NALU_TYPE_SPS = 7,
    NALU_TYPE_PPS = 8,
    NALU_TYPE_AUD = 9,
    NALU_TYPE_EOSEQ = 10,
    NALU_TYPE_EOSTREAM = 11,
    NALU_TYPE_FILL = 12,
} NaluType;

typedef enum {
    NALU_PRIORITY_DISPOSABLE = 0,
    NALU_PRIRITY_LOW = 1,
    NALU_PRIORITY_HIGH = 2,
    NALU_PRIORITY_HIGHEST = 3
} NaluPriority;


typedef struct
{
    int startcodeprefix_len;      //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)  
    unsigned len;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)  
    unsigned max_size;            //! Nal Unit Buffer size  
    int forbidden_bit;            //! should be always FALSE  
    int nal_reference_idc;        //! NALU_PRIORITY_xxxx  
    int nal_unit_type;            //! NALU_TYPE_xxxx    
    int info2 ;
	int info3 ;
    char *buf;                    //! contains the first byte followed by the EBSP  
} NALU_t;
typedef struct
{  
	int    nWidth;  
	int    nHeight;  
	int    nFrameRate;      
	unsigned int    nSpsLen;  
	unsigned char   *Sps;  
	unsigned int    nPpsLen;  
	unsigned char   *Pps;   
	int 	idr_offset;
	int 	isparser;
} metavideo_t;  
typedef struct
{  
	unsigned short AudioSpecificConfig;  
	
} metaaudio_t;

typedef struct
{
	char *buf;
	int	 offset;
	int  size;
} buffer_t;
typedef  unsigned int UINT;
typedef  unsigned char BYTE;
typedef  unsigned long DWORD;

UINT Ue(BYTE *pBuff, UINT nLen, UINT &nStartBit);



int Se(BYTE *pBuff, UINT nLen, UINT &nStartBit);



DWORD u(UINT BitCount,BYTE * buf,UINT &nStartBit);


/**
 * H264的NAL起始码防竞争机制 
 *
 * @param buf SPS数据内容
 *
 * @无返回值
 */ 
void de_emulation_prevention(BYTE* buf,unsigned int* buf_size);


/**
 * 解码SPS,获取视频图像宽、高信息 
 *
 * @param buf SPS数据内容
 * @param nLen SPS数据的长度
 * @param width 图像宽度
 * @param height 图像高度

 * @成功则返回1 , 失败则返回0
 */ 
int h264_decode_sps(BYTE * buf,unsigned int nLen,int &width,int &height,int &fps);
typedef int (*get_data_cb)(unsigned char *buf,int size,void *data);
int get_data_from_buffer(unsigned char *buf,int size,void *data);
int get_data_from_file(unsigned char *buf,int size,void *data);
int GetH264NALU(NALU_t *nalu,void *usr_data,get_data_cb get);
int GetMetaNALU(metavideo_t *metaData,NALU_t *nalu,void *usr_data,get_data_cb get);
int parser_video_meta(unsigned char *data,unsigned int size,metavideo_t *meta);
int Parsr_H264_Nalu_List(char *url,int max,NALU_t **nalu,metavideo_t *meta);
int aes_decrypt(const char *aes_key,unsigned char *input, unsigned char *output);


#endif



