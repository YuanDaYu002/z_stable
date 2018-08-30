#ifndef AMBA_H

#define AMBA_H

#define PubGetSysParameter GetSysParameter
#define UPGRADECHECKSTRUCT UpgradeFileHead

#include "interfacedef.h"
#include "common.h"
#include "EncodeManage.h"
typedef struct
{
    PTZ_CMD_E cmd;
    unsigned short para0;
    unsigned short para1;
} PTZ_OPERATE_PARAM;

typedef enum
{
    BLK_DEV_ID_HDD00 = 0,//固定的存储è      ¾备，HDD,SD,TF等
    BLK_DEV_ID_HDD01,
    BLK_DEV_ID_HDD02,
    BLK_DEV_ID_HDD03,
    BLK_DEV_ID_UDISK0,//可插拔的存储设备，u盘，暂不支持
    BLK_DEV_ID_UDISK1,
    BLK_DEV_ID_MAX_NUM//最大支持块设备的数          量
}BLK_DEV_ID;

typedef struct
{
    unsigned int magic;
    unsigned char uboot_version[16];
    unsigned char kernel_version[16];
    unsigned char rootfs_version[16];
    unsigned char app_version[16];
    unsigned char hardware_version[32];
    unsigned char date[16];
    unsigned int md5[4];
    unsigned int file_len;
    unsigned int upgrade_flag;
    unsigned int offset_uboot;
    unsigned int len_uboot;
    unsigned int offset_kernel;
    unsigned int len_kernel;
    unsigned int offset_rootfs;
    unsigned int len_rootfs;
    unsigned int offset_app;
    unsigned int len_app;
    unsigned char m_reserved[84];
} UpgradeFileHead; // 256 bytes

int ForceIdrInsertion(int ch,STREAM_TYPE_E enStreamTpye);

int IsBootByPir();

int Get_Wifi_Status(void);

int GetSysParameter(int type, void * para);

int GetWifiRouterMac(char *buf);

void RestoreDefault();

int ResetUserData2IFrameBySecond(int channel,int streamtype, int userid, int seconds);

int Crtl_ptzPara(int cmd,PTZ_OPERATE_PARAM *p);

int StartUpdate(UpgradeFileHead *phead);

int GetBlockDeviceInfo(BLK_DEV_ID enBlkDevId, BlockDevInfo_S *pstBlockDevInfo);

int CheckUpdateVersion(UpgradeFileHead *pHead);

int CheckUpdateFileMD5(UpgradeFileHead *pHead, FILE *fp);

//#define STRUCT_SET_PTZ_REQUEST PTZ_OPERATE_PARAM

#endif /* end of include guard: AMBA_H */
