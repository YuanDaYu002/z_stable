
#ifndef _ENCODE_MANAGE_H
#define _ENCODE_MANAGE_H

#include "BufferManage.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum
{
    STREAM_TYPE_HIGHT = 0,
    STREAM_TYPE_MEDIUM = 1,
    STREAM_TYPE_LOW = 2,
    STREAM_TYPE_JPEG = 3
}STREAM_TYPE_E;

typedef enum
{
  SNAPSHOT_JPEG_TYPE_1920X1080 = 0,
  SNAPSHOT_JPEG_TYPE_640X480 = 1,
  SNAPSHOT_JPEG_TYPE_320X240 = 2,
  SNAPSHOT_JPEG_TYPE_640X360 = 3
} SNAPSHOT_JPEG_TYPE_E;

typedef struct 
{
	int 			sendflag;
	int 			Result;
	char 			path[96];/*路径  eg : /tmp*/
	char			FileName[64];/*抓拍输入的文件名称,email_pic.jpg  文件名邮件已经使用*/
	char			FullFileName[128];/*返回的全路径 eg:   /tmp/email_pic.jpg  */
	datetime_setting 	 m_DateTime;
	int			m_SendingPic;

}SnapImageArg;



int SnapOneChannelImage(int ch, SnapImageArg *Snap, SNAPSHOT_JPEG_TYPE_E cap_type);

#ifndef AMBA
int GetSendNetFrame(int channel,int streamtpye, int userid, unsigned char **buffer, FrameInfo *m_frameinfo);
int ResetUserData2IFrame(int channel,int streamtype, int userid);
int ResetUserData2CurrentIFrame(int channel,int streamtype, int userid);
int ResetUserData2LastIFrame(int chanel, int streamtype, int userid);
int	StartAudioDecode(int Audiotype);
int	StopAudioDecode();
int	SendAudioStreamToDecode(unsigned char *buffer,int len, int block=0);
int SnapshotPic(char *filename,SNAPSHOT_JPEG_TYPE_E enSnapshotJpegType);
int ResetUserData2IFrameBySecond(int channel,int streamtype, int userid, int seconds);

#endif

int RequestIFrame(int streamtype,int time);

int	StartAudioEncode(int Audiotype);

int	StopAudioEncode();

bool AudioGetSpeeker();

bool AudioReleaseSpeeker();

#ifdef __cplusplus
 };
#endif

#ifdef AMBA
int GetSendNetFrame(int channel,STREAM_TYPE_E streamtpye, int userid, unsigned char **buffer, FrameInfo *m_frameinfo);
int ResetUserData2IFrame(int channel,int streamtype, int userid);
int ResetUserData2CurrentIFrame(int channel,int streamtype, int userid);
int ResetUserData2LastIFrame(int chanel, int streamtype, int userid);
int	StartAudioDecode(int Audiotype);
int	StopAudioDecode();
int	SendAudioStreamToDecode(unsigned char *buffer,int len);
int SnapshotPic(char *filename,SNAPSHOT_JPEG_TYPE_E enSnapshotJpegType);

#endif

#endif

