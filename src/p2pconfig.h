#ifndef P2PCONFIG_H
#define P2PCONFIG_H

//if define, use syslog instead printf
#ifndef ANDROID
#define SYSLOG 1
#endif
//if use assert
#define USE_ASSERT

//if support remote playback
#define PLAYBACK

#define STUN_DIR        "/tmp/stun"

//setup  UDT
#define  P2P_MTU  	  1200
#define  P2P_SNDBUF   300*1024
#define  P2P_RCVBUF   80*1024

//for p2psession manager
#define  MAX_P2P_SESSIONS 10

//for limit the max frame size
#define  MAX_FRAME_SIZ  512*1024
//for frame slic
#define  RTP_SLIC_SIZE  1000

/*
	P2P_ASSERT( ptr != NULL );
	P2P_ASSERT( ptr != NULL , -1); -1在不使用assert的时候当做函数返回值返回
*/
#ifdef   USE_ASSERT
#define  P2P_ASSERT(x,...)   	assert(x)
#else
#define  P2P_ASSERT(x,...) \
	do{\
		if(!(x)) {\
			plog("ERROR: assert(%s) failed!!!\n", #x);\
			return __VA_ARGS__;\
		}\
	}while(0)
#endif

//#define  DEV_ACCESS             "http://50.226.99.160:80,http://50.226.99.161:80,http://50.226.99.168:80,http://50.226.99.169:80"
#define  DEV_ACCESS             "https://11-DevAccess.myzmodo.com,https://12-DevAccess.myzmodo.com,https://13-DevAccess.myzmodo.com,https://14-DevAccess.myzmodo.com"
#define  UPGRADE_SERVER         "devupgrade.meshare.com"

#define  WEB_LOGIN_URI      	"/factorydevice/devlogin"
#define  WEB_REPORT_URI    	 	"/factorydevice/confrpt"
#define  WEB_SYNC_URI      		"/factorydevice/sync"
#define  WEB_RECORD_REPORT_URI	"/factorydevice/devrcdlistrpt"
#define  WEB_SD_FORMAT_URI	  	"/factorydevice/sd_format"
#define  WEB_GET_TIMEZONE_URI	"/factorydevice/gettimezone"
#define  WEB_REPORT_UPNP_URI 	"/factorydevice/upnp_rpt"
#define  WEB_TIMELINE_RPT_URI	"/factorydevice/timeline_rpt"
#define  WEB_PRESET_SET_URI		"/factorydevice/preset_set"
#define  WEB_GET_VOICEMSG_URI   "/factorydevice/get_voice_message"
#define	 WEB_ADDING_DEVICE_URI	"/factorydevice/adding_passive_devs_rpt"
#define  WEB_SET_UPGRADE_URI    "/upgrade/setst"
#define  WEB_REST_RPT_URI		"/factorydevice/rest_rpt"
#define  WEB_ALARM_URI     		"/message/msgnotify"
#define  WEB_GET_TIMEZONELIST   "/common/timezonelist"
#define  WEB_REPORT_PICTURE_URI	"/fileserver/cover_report"
#define  FILESERVER_UPLOAD_URI	"/fileserver/dev_upload_file"
#define  WEB_PRESET_COVER_URI   "/fileserver/preset_cover_report"
#define  WEB_GETFILE_URI        "/fileserver/get_file"
#define  WEB_UPLOADFILE_URI 	"/fileserver/com_upload_file"
#define  WEB_UPDATE_ALERT_URI   "/internal/msg_update"
#define  WEB_LPR_RPT            "/fileserver/lpr_rpt"
#define  WEB_FACE_RPT           "/fileserver/face_report"

#include "test_config.h"
#include <string>

#define  WEB_LOGIN_URL          (std::string(get_web_server()) + std::string(WEB_LOGIN_URI)).c_str()
#define  WEB_SET_UPGRADE_STATE 	(std::string(UPGRADE_SERVER) + std::string(WEB_SET_UPGRADE_URI)).c_str()

#define  STREAMTYPE_QVGA 0
#define  STREAMTYPE_VGA  1
#define  STREAMTYPE_720P 2
#define  STREAMTYPE_RECORD 3

#endif /* end of include guard: P2PCONFIG_H */

