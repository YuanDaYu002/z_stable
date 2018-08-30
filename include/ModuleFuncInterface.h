
#ifndef __MODULE_FUNC_INTERFACE_H__

#define __MODULE_FUNC_INTERFACE_H__

#include "interfacedef.h"
#include "systemparameterdefine.h"
#include "upgrademodule.h"
#include "BufferManage.h"   
#include "p2p_interface.h"
#include "p2p_alarm.h"

void RestoreDefault(void *para);

int PubSetSysParameter(int type, void * para);

int PubGetSysParameter(int type, void * para);

#ifndef AMBA
int GetBlockDeviceInfo(int type, BlockDevInfo_S * Info);
#endif

int GetSoftWareVersion(TYPE_DEVICE_INFO *version);

int PTZCtrlAction(int ch, int cmd, int para0, int para1);

void RebootSystem();

int SetVideoFlipMirror(int mode);

bool GetSpeeker();

bool ReleaseSpeeker();


/*add by hayson begin 2013.12.26*/
/*返回无线网卡名字*/
const char* get_wifi_name();
/*返回有线网卡名字*/
const char* get_local_name();
/*返回MAC和二维码配置文件路径*/
const char* get_mac_id_file();
/*返回IE插件版本*/
const char* get_ie_version();

/* add by mike 2014-01-09*/
const char* get_app_version();

const char* get_uboot_version();

const char* get_kernel_version();

const char* get_fs_version();

int Crtl_ptzPara(int cmd,STRUCT_SET_PTZ_REQUEST *pReq);

void RestartNtpClient(int timezone_offset);

int OnSetTimezoneByMeshare(int timezone_index, int timezone_offset);

int webserver_get_device(web_sync_param_t* sync_data);
int webserver_set_device(web_sync_param_t* sync_data);

void BlockWaitForAlarms();

struct p2p_broadcast_alarm_t
{
	int alarm_type;
	int channel;
    void *alarm_info;
};

void BroadcastAlarmsEx(P2pAlarmType alarm_type, int chl);

int  P2pGetOneAlarm(p2p_broadcast_alarm_t * alarm);

int  FormatSD(int *process);

int GetWebScheduleSwitch(int type);

int GetPTZResetPoint(char *buffer,int *len);

int NtpIfEnable();
#endif 
