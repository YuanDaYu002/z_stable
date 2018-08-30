#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include "zsp_h.h"
#include "p2p_def.h"
#include "device_operation.h"
#include "plog.h"
#include "zmutex.h"
#include "web.h"
#include "zjson.h"
#include "zmdtimer.h"
#include "helpfunction.h"
#include "p2p_voice_message.h"
#include "zmd_task.h"
#include "p2p_interface.h"
#ifdef AMBA
#include "amba.h"
#else
#define BLK_DEV_ID_HDD00	0
#endif

extern std::map<std::string, config_key_cb_t> g_config_key_callback;

void parser_schedule_list(char * schedule_list, web_sync_param_t*);

typedef struct _mic_holder_t {
    ZMutex *lock;
    int is_free;
    void *linkid;
    char uid[128];
} mic_holder_t;
static mic_holder_t mic_holder = {(new ZMutex), TRUE, NULL};
p2p_init_info_t g_p2p_envir;

int  p2p_set_init_info(p2p_init_info_t* info, unsigned int info_len)
{	
#define COPY_ENVIR_STRING(EVIR_NAME)\
    do{\
        if(info->EVIR_NAME) g_p2p_envir.EVIR_NAME = strdup(info->EVIR_NAME);\
        else g_p2p_envir.EVIR_NAME = strdup("");\
    }while(0)

    memcpy(&g_p2p_envir, info, info_len);

    COPY_ENVIR_STRING(device_id);
    COPY_ENVIR_STRING(config_dir);
    COPY_ENVIR_STRING(uboot_version);
    COPY_ENVIR_STRING(kernel_version);
    COPY_ENVIR_STRING(rootfs_version);
    COPY_ENVIR_STRING(app_version);
    COPY_ENVIR_STRING(device_name);
    COPY_ENVIR_STRING(network_interface);
    COPY_ENVIR_STRING(high_resolution);
    COPY_ENVIR_STRING(secondary_resolution);
    COPY_ENVIR_STRING(low_resolution);
    COPY_ENVIR_STRING(voice_message_path[0]);
    COPY_ENVIR_STRING(voice_message_path[1]);
    COPY_ENVIR_STRING(voice_message_path[2]);
    COPY_ENVIR_STRING(voice_message_path[3]);
    COPY_ENVIR_STRING(voice_message_path[4]);
    COPY_ENVIR_STRING(aes_key);
	COPY_ENVIR_STRING(module_id_915);

    return 0;
#undef COPY_ENVIR_STRING
}

//get io alarm channel number
int p2p_get_device_io_chlnum()
{
    return g_p2p_envir.device_alarm_in_num;
}
int p2p_is_channel_io_alarm(int ionum, int chlnum)
{
#ifdef _BUILD_FOR_NVR_
    ALARMZONEGROUPSET AlarmZoneSet;
    PubGetSysParameter(SYSALARMZONE_SET, (void *)&AlarmZoneSet);
    if(ionum > p2p_get_device_io_chlnum())
        return 0;
    return TEST_BIT(AlarmZoneSet.m_AlarmZone[ionum].m_u32ShotEnable, chlnum);
#else
    return 0;
#endif
}

int  p2p_hold_mic(void* linkid, const char* uid)
{
    plogfn();

    ZMutexLock l(mic_holder.lock);
    if(mic_holder.is_free)
    {
#if !defined(_BUILD_FOR_NVR_)
        if( StartAudioDecode( 0 ) != 0 )
            return -1 ;
        if( !GetSpeeker() )
        {
            plog("GetSpeeker() failed!\n");
            return -1;
        }
#endif
        mic_holder.is_free = FALSE;
        mic_holder.linkid = linkid;
        if(uid && uid != linkid)
            p2p_strncpy(mic_holder.uid, uid, sizeof(mic_holder.uid));

        plog("hold mic success\n");
        return 0;
    }
    else
    {	/* 同一个客户端 重连成功，更新信息*/
        if(uid && strcmp(uid, mic_holder.uid)==0)
        {
            mic_holder.linkid = linkid;
            plog("continue hold mic success\n");
            return 0;
        }
        plog("hold mic failed\n");
        return -1;
    }
    return -1;

}
int p2p_free_mic(void* linkid, const char* uid)
{
    plogfn();

    ZMutexLock l(mic_holder.lock);
    if(mic_holder.is_free)
        return -1;
    /* 当linkid和uid相等时，表示强制释放*/
    if((uid && strcmp(uid, mic_holder.uid)==0)|| 
            linkid == mic_holder.linkid || linkid == uid)
    {		
        mic_holder.is_free = TRUE;

#if !defined(_BUILD_FOR_NVR_)
        StopAudioDecode();
        ReleaseSpeeker();
#endif
        return 0;
    }
    return -1;
}
void p2p_decode_audio(void* data, int len)
{
    if(len != 164)
        plog("len:%d\n", len);
#if !defined(_BUILD_FOR_NVR_)
    if(0 != SendAudioStreamToDecode( (unsigned char*)data , len))
        plog("decode audio failed!!!\n");
#endif //_BUILD_FOR_NVR_
}

int  p2p_is_upnp_success()
{
    //plog("\n");
    return CNetServer::getInstance()->IsUpnpSuccess();
}

int  p2p_get_upnp_video_port()
{
    NETWORK_PARA netset;

    PubGetSysParameter(SYSNET_SET,&netset);

    if(p2p_is_upnp_success())
    {
        return netset.m_Eth0Config.m_v_port;
    }
    else
    {
        //0 indicate failed
        return 0;
    }
}

int p2p_get_local_video_port()
{
    NETWORK_PARA netset;

    PubGetSysParameter(SYSNET_SET,&netset);

    return netset.m_CenterNet.m_uVideoListenPt;
}

int p2p_get_usrid_type(int request_type)
{
    switch(request_type)
    {
        case STREAMTYPE_VGA:
            return  VGA_CHN_TYPE;
        case STREAMTYPE_QVGA:
            return  QVGA_CHN_TYPE;
        case STREAMTYPE_720P:
            return  D720P_CHN_TYPE;
        default:
            break;
    }
    return 1;
}

int p2p_get_stream_type(int request_type)
{
#ifndef _BUILD_FOR_NVR_
    switch(request_type)
    {
        case STREAMTYPE_VGA:
            return  1 ;
        case STREAMTYPE_QVGA:
            return 2 ;
        case STREAMTYPE_720P:
            return 0 ;
        case STREAMTYPE_RECORD:
            return 3;
        default :
            break;
    }

    return 2;//default qvga
#else
    //for nvr
    switch(request_type)
    {
        case STREAMTYPE_VGA:
            return  VGA_CHN_TYPE;
        case STREAMTYPE_QVGA:
            return  QVGA_CHN_TYPE;
        case STREAMTYPE_720P:
            return D720P_CHN_TYPE;
        case STREAMTYPE_RECORD:
            return 3;
        default:
            break;
    }

    return QVGA_CHN_TYPE;//default qvga
#endif
}
/***************************************************
 * streamtype: 0 qvga, 1 vga, 2 720p
 **************************************************/
int p2p_is_channel_valid(int chlnum, int streamtype)
{
    if(chlnum < 0 || chlnum > 64)
        return 0;
#ifdef _BUILD_FOR_NVR_

    if(streamtype == STREAMTYPE_720P && !GetNetModule()->IsIpcClientValid( chlnum ,  D720P_CHN_TYPE) &&
    			!GetNetModule()->IsIpcClientValid( chlnum ,  D1080P_CHN_TYPE))
    {
    	plog("get stream falied!!!\n");
    	return 0;
    }
    else if( !GetNetModule()->IsIpcClientValid( chlnum ,  p2p_get_stream_type(streamtype)) )
    {
        plog("get stream falied!!!\n");
        return 0;
    }
#else
    if(chlnum != 0)
        return 0;
#endif
    return 1;
}
int p2p_get_device_chlnum()
{
    return g_p2p_envir.device_video_num;
}
int p2p_ptz_operat(int chl, int ptz_cmd, int para0, int para1)
{
#ifndef _BUILD_FOR_NVR_
#ifndef AMBA
    STRUCT_SET_PTZ_REQUEST req;
#else
    PTZ_OPERATE_PARAM req;
#endif
    req.cmd = (PTZ_CMD_E)ptz_cmd;
    req.para0 = para0;
    req.para1 = para1;

    return Crtl_ptzPara(ptz_cmd, &req);
#else
    //only for DVR
    PTZCtrlAction(chl, ptz_cmd, para0, para1);
    return 0;
#endif
}

int p2p_ptz_get_preset(char* buf, int* len)
{
#ifndef _BUILD_FOR_NVR_
    return GetPTZResetPoint(buf, len);
#else
    return 0;
#endif
}
const char* p2p_get_device_capture(char* pic_path, int chl, int type)
{
#ifndef _BUILD_FOR_NVR_
    SNAPSHOT_JPEG_TYPE_E cap_type;

    if(!p2p_is_device_on())
        return NULL;

    if(type == 0)
        cap_type = SNAPSHOT_JPEG_TYPE_640X360;
    else
        cap_type = SNAPSHOT_JPEG_TYPE_320X240;

    if(0 == SnapshotPic(pic_path, cap_type))
    {
        return pic_path;
    }
    return NULL;
#else
    pic_path[0] = '\0';
    if(HandleSnapJpg(chl, 5, pic_path ))
        return NULL;
    if(strlen(pic_path) == 0)
        return NULL;
    return pic_path;
#endif
}

bool p2p_is_p2p_device()
{
    return p2p_check_bit(g_p2p_envir.device_capacity, 8);
}

const char* p2p_get_deviceid()
{
    return g_p2p_envir.device_id;
}

int p2p_nvr_get_valid_buftype(int chlnum, int streamtype)
{
#ifdef _BUILD_FOR_NVR_
    plogfn();
    int bufType = GetMediaBufIndex( chlnum , streamtype );
    if(bufType != -1 )
        return bufType;

    int d1080p_type = GetMediaBufIndex( chlnum , D1080P_CHN_TYPE ) ;
    int d720p_type = GetMediaBufIndex( chlnum , D720P_CHN_TYPE ) ;
    int vga_type = GetMediaBufIndex( chlnum , VGA_CHN_TYPE ) ;
    int qvga_type = GetMediaBufIndex( chlnum , QVGA_CHN_TYPE ) ;
    if(streamtype == D720P_CHN_TYPE)
    {
        if(d1080p_type != -1) return d1080p_type;
        if(vga_type != -1) return vga_type;
        if(qvga_type != -1) return qvga_type;
    }
    else if(streamtype == VGA_CHN_TYPE)
    {
        if(qvga_type != -1) return qvga_type;
        if(d1080p_type != -1) return d1080p_type;
        if(d720p_type != -1) return d720p_type;
    }
    else if(streamtype == QVGA_CHN_TYPE)
    {
        if(vga_type != -1) return vga_type;
        if(d720p_type != -1) return d720p_type;
        if(d1080p_type != -1) return d1080p_type;
    }
#endif
    return -1;
}
int p2p_get_device_type()
{
    return g_p2p_envir.device_type;
}
uint32_t p2p_get_device_capacity()
{
    return g_p2p_envir.device_capacity;
}
uint32_t p2p_get_device_extend_capacity()
{
    return g_p2p_envir.device_extend_capacity;
}
unsigned long long p2p_get_device_supply_capacity()
{
    return g_p2p_envir.device_supply_capacity;
}
const char* p2p_get_device_versions(char *versions)
{
    sprintf(versions, "%s;%s;%s;%s",
            g_p2p_envir.uboot_version, g_p2p_envir.kernel_version,
            g_p2p_envir.rootfs_version,	g_p2p_envir.app_version);
    return versions;
}
const char* p2p_get_device_name()
{
    return  g_p2p_envir.device_name;
}
int p2p_force_i_frame(int chl, int streamtype)
{
    plog("chl :%d, streamtype:%d\n", chl, streamtype);

    if(!p2p_is_channel_valid(chl, streamtype))
        return -1;

#ifndef _BUILD_FOR_NVR_
#ifdef AMBA
    return ForceIdrInsertion(0,(STREAM_TYPE_E)(p2p_get_stream_type(streamtype)));
#else
    return RequestIFrame(p2p_get_stream_type(streamtype), 0);
#endif

#else
    //dvr
    if(2 ==  p2p_get_device_type())
    {
        if(streamtype == STREAMTYPE_VGA || streamtype == STREAMTYPE_QVGA)
            streamtype = VGA_CHN_TYPE;
        else if(streamtype == STREAMTYPE_720P)
            streamtype = D720P_CHN_TYPE;
        else 
            streamtype = VGA_CHN_TYPE;
        return RequestIFrame(chl, (ZMD_CHN_TYPE)streamtype);
    }
    //nvr
    else if( 1 ==  p2p_get_device_type())
    {
        char ip[32];
        unsigned short port;

        if(streamtype == STREAMTYPE_VGA)
            streamtype = 1;
        else if(streamtype == STREAMTYPE_QVGA)
            streamtype = 2; 
        else if(streamtype == STREAMTYPE_720P)
            streamtype = 0;
        else 
            streamtype = 2;
        if( CIPCManager::getInstance()->getChnIpAddr( chl , ip , port ) )
        {
            if(CNetModule::getInstance()->ForceIFraram( ip , port, streamtype))
                return 0;
        }
    }
    return -1;
#endif
}

const char* p2p_get_config_dir()
{
    return g_p2p_envir.config_dir;
}

struct P2PMediaInfo
{
    int chlnum;
    int channelid;
    int streamtype;
    int mediaid;
#ifdef _BUILD_FOR_NVR_
    int buftype; 
#endif
};

void* p2p_get_media_id(int chlnum, int streamtype)
{
    P2PMediaInfo *pInfo = new P2PMediaInfo;
    pInfo->chlnum = chlnum;
    pInfo->streamtype = p2p_get_stream_type(streamtype);
    pInfo->channelid  = p2p_get_usrid_type(streamtype);
#ifdef _BUILD_FOR_NVR_
    pInfo->buftype = p2p_nvr_get_valid_buftype(chlnum , pInfo->streamtype); 
#endif
    pInfo->mediaid = GetMediaMgr()->getUnuseMediaSession(chlnum, pInfo->channelid);
    if(pInfo->mediaid < 0)
    {
        delete pInfo;
        return NULL;
    }

    plog("get mediaid:[%u][%d]!!!!!!\n", (unsigned int)pInfo, pInfo->mediaid);

    return pInfo;
}
void p2p_free_media_id(void* id)
{
    if(id == NULL)
        return;

    P2PMediaInfo *pInfo = (P2PMediaInfo *)id;
    GetMediaMgr()->freeMediaSession(pInfo->chlnum, pInfo->channelid, pInfo->mediaid);
    if(pInfo)
        delete pInfo;
}
int p2p_set_i_frame(void* id)
{
    P2PMediaInfo *pInfo = (P2PMediaInfo *)id;

#ifndef _BUILD_FOR_NVR_
#ifdef AMBA
    static int flag_pirboot = 0;
    if (0 == flag_pirboot && IsBootByPir())
    {
        plog("media info:(channelid:%d,chlnum:%d,mediaid:%d,streamtype:%d)\n",pInfo->channelid,pInfo->chlnum,pInfo->mediaid,pInfo->streamtype);
        flag_pirboot = 1;
        return ResetUserData2IFrameBySecond(pInfo->chlnum, pInfo->streamtype, pInfo->mediaid, 30);
    }
#endif
    return ResetUserData2IFrame(pInfo->chlnum, pInfo->streamtype, pInfo->mediaid);
#else
    return ResetUserData2IFrame(pInfo->chlnum, pInfo->buftype , pInfo->mediaid); 
#endif
}
int p2p_set_current_i_frame(void* id)
{
    P2PMediaInfo *pInfo = (P2PMediaInfo *)id;

#ifndef _BUILD_FOR_NVR_
    return ResetUserData2CurrentIFrame(pInfo->chlnum, pInfo->streamtype, pInfo->mediaid);
#else
    return ResetUserData2IFrame(pInfo->chlnum, pInfo->buftype , pInfo->mediaid); 
    //return ResetUserData2CurrentIFrame(pInfo->chlnum, pInfo->buftype , pInfo->mediaid); 
#endif
}
int p2p_set_i_frame_by_curpos(void* id)
{
	P2PMediaInfo *pInfo = (P2PMediaInfo *)id;
#ifndef _BUILD_FOR_NVR_
	return ResetUserData2LastIFrame(pInfo->chlnum, pInfo->streamtype, pInfo->mediaid);
#else
	return -1;
#endif
}
int p2p_set_i_frame_by_second(void* id, int sec)//sec <=0 由应用层来决定秒数
{
	P2PMediaInfo *pInfo = (P2PMediaInfo*)id;
#ifndef _BUILD_FOR_NVR_
	return ResetUserData2IFrameBySecond(pInfo->chlnum, pInfo->streamtype, pInfo->mediaid, sec);
#else
	return ResetUserData2IFrame(pInfo->chlnum, pInfo->buftype , pInfo->mediaid);
#endif
}
int p2p_get_one_frame(void* id, unsigned char** data, void *frameInfo)
{
    P2PMediaInfo *pInfo = (P2PMediaInfo *)id;
#ifndef _BUILD_FOR_NVR_
    return GetSendNetFrame(pInfo->chlnum, (STREAM_TYPE_E)pInfo->streamtype, 
            pInfo->mediaid,data, (FrameInfo*)frameInfo) ;
#else
    return GetSendNetFrameByBufType(pInfo->chlnum, pInfo->buftype, 
            pInfo->mediaid,data, (FrameInfo*)frameInfo);
#endif
}

int p2p_get_sd_card_status(SD_CARD_STAT *st)
{
    BlockDevInfo_S	storageDev;

    memset(&storageDev, 0 , sizeof(storageDev));

    GetBlockDeviceInfo(BLK_DEV_ID_HDD00, &storageDev);
    //plog("storageDev.m_u8Exist == %d\n", storageDev.m_u8Exist);
    if( storageDev.m_u8Exist == 2 )
        st->status = 0;
    else if( storageDev.m_u8Exist == 0 )
        st->status = 1;
    else if( storageDev.m_u8Exist == 1 )
        st->status = 2;
    else if( storageDev.m_u8Exist == 3 )
        st->status = 3;
    else
        st->status = 1;

    st->free_size = 0;
    st->used_size = 0;
    if(st->status == 0)
    {
        st->free_size = storageDev.m_u32FreeSpace;
        st->used_size = storageDev.m_u32Capacity - st->free_size;
    }
    return 0;
}

int p2p_send_cover_pic( int channel )
{
    char fpath[128];
    int  ret = -1;
    if(p2p_get_device_capture(fpath, channel))
    {
        plog("begin upload cover picture for channel[%d]...\n", channel);
        ret = web_upload_cover_picture(fpath, channel);
        unlink(fpath);
    }
    return ret;
}
int p2p_format_sd_card()
{
#ifndef _BUILD_FOR_NVR_
    int format_progress = 0;
    return FormatSD(&format_progress);
#else
    return 0;
#endif
}

static const char* meshare_timezone_index[] = {
    "",
    "Etc/GMT+12",
    "Pacific/Samoa",
    "Pacific/Honolulu",
    "America/Juneau",
    "America/Santa_Isabel",
    "America/Los_Angeles",
    "America/Chihuahua",
    "America/Denver",
    "America/Phoenix",
    "America/Mexico_City",
    "America/Regina",
    "America/Chicago",
    "America/Belize",
    "America/Bogota",
    "America/New_York",
    "America/Indianapolis",
    "America/Caracas",
    "America/Halifax",
    "America/Manaus",
    "America/Guyana",
    "America/Santiago",
    "America/Asuncion",
    "America/St_Johns",
    "America/Sao_Paulo",
    "America/Argentina/Buenos_Aires",
    "America/Godthab",
    "America/Cayenne",
    "America/Montevideo",
    "America/Noronha",
    "Atlantic/Cape_Verde",
    "Atlantic/Azores",
    "Europe/London",
    "Africa/Casablanca",
    "Atlantic/Reykjavik",
    "UTC",
    "Europe/Berlin",
    "Europe/Budapest",
    "Europe/Paris",
    "Europe/Warsaw",
    "Africa/Douala",
    "Asia/Amman",
    "Asia/Beirut",
    "Africa/Harare",
    "Europe/Helsinki",
    "Africa/Cairo",
    "Europe/Minsk",
    "Africa/Windhoek",
    "Europe/Athens",
    "Asia/Jerusalem",
    "Asia/Baghdad",
    "Asia/Tbilisi",
    "Asia/Riyadh",
    "Europe/Moscow",
    "Africa/Nairobi",
    "Asia/Tehran",
    "Asia/Dubai",
    "Asia/Yerevan",
    "Asia/Baku",
    "Indian/Mauritius",
    "Asia/Kabul",
    "Asia/Ashgabat",
    "Asia/Yekaterinburg",
    "Asia/Karachi",
    "Asia/Calcutta",
    "Asia/Colombo",
    "Asia/Katmandu",
    "Asia/Almaty",
    "Asia/Dacca",
    "Asia/Rangoon",
    "Asia/Krasnoyarsk",
    "Asia/Bangkok",
    "Asia/Shanghai",
    "Asia/Kuala_Lumpur",
    "Australia/Perth",
    "Asia/Taipei",
    "Asia/Ulaanbaatar",
    "Asia/Tokyo",
    "Asia/Seoul",
    "Asia/Yakutsk",
    "Australia/Adelaide",
    "Australia/Darwin",
    "Australia/Brisbane",
    "Asia/Vladivostok",
    "Pacific/Port_Moresby",
    "Australia/Hobart",
    "Australia/Canberra",
    "Pacific/Guadalcanal",
    "Pacific/Auckland",
    "Asia/Kamchatka",
    "Pacific/Fiji",
    "Pacific/Tongatapu",
    NULL,
};
//tid is time zone id
int p2p_get_local_timezone_index_from_meshare_tid(const char* time_zone_id)
{
    for(int i=0; i<sizeof(meshare_timezone_index)/sizeof(char*); i++)
    {
        if(meshare_timezone_index[i])
        {
            if(0 == strcasecmp(time_zone_id, meshare_timezone_index[i]))
                return i;
        }
    }
    return -1;
}
//tid is time zone id
const char* p2p_get_meshare_tid_from_local_timezone_index(int index)
{
    if(index > sizeof(meshare_timezone_index)/sizeof(char*) || index < 0)
        return NULL;
    return meshare_timezone_index[index];
}
int p2p_set_device_timezone(const char* timezon_id, int offset)
{
    int tid = p2p_get_local_timezone_index_from_meshare_tid(timezon_id);
    if(tid == -1)
        return tid;
    return OnSetTimezoneByMeshare(tid, offset);
}

int p2p_set_timezone_offset(int offset)
{
    plogfn();

    web_sync_param_t local_data;

    p2p_get_sync_paramter(&local_data);

    if(offset != local_data.time_offset)
    {
        p2p_set_device_timezone(local_data.time_zone, offset);
    }
    return 0;
}

int convert_int_time(const char *pTime)
{
    unsigned short  iResult = 0;
    int hour=0,min=0;
    int iRet = sscanf(pTime,"%d:%d",&hour,&min);
    if(iRet == 2)
    {
        iResult = hour*60+min;
    }
    return iResult;
}

static void  p2p_convert_schedule(void* para)
{
    char* json_list = (char*)para;
    if(strstr(json_list, "[") == NULL)
    {
        free(json_list);
        return ;
    }
    int len = json_array_get_length(json_list);

    p2p_schedule_t *list = (p2p_schedule_t*)malloc(len*sizeof(p2p_schedule_t));

    if(!list)
    {
        free(json_list);
        return ;
    }

    memset(list, 0, len*sizeof(p2p_schedule_t));

    for(int i=0; i<len; i++)
    {
        const char* js = json_array_get_object_by_idx(json_list, i);
        if(js)
        {
            JsonParser jp(js);
            jp.GetIntValue(list[i].flag, "flag");
            list[i].off_at = convert_int_time(jp.GetStringConst("off_at"));
            list[i].on_at = convert_int_time(jp.GetStringConst("on_at"));
            jp.GetIntValue(list[i].repeat_day, "repeat_day");
            jp.GetIntValue(list[i].schedule_id, "schedule_id");
        }
    }
    if (g_p2p_envir.on_set_schedule)
        g_p2p_envir.on_set_schedule(list, len);
    free(list);
    free(json_list);
    return; 
}
int conver_time_list_to_global(int index, const char *schedule_list, web_sync_param_t *para)
{
    if(index < 0 || index > 9 || schedule_list == NULL ||  para->time_list == NULL)
        return -1;

    char tmp_time[64] = {0};
    int  flag = -1;

    json_object_parser((char*)schedule_list, "flag%64s", tmp_time);
    if(strlen(tmp_time) == 0)
        return -1;
    else 
        flag = atoi(tmp_time);

    /* 过滤设备不需要存储的时间段 */
    if(flag == -1 || flag == 1 ||  flag == 2)
        return -1;

    para->time_list[index].flag = flag;

    memset(&tmp_time, 0x0, sizeof(tmp_time));

    json_object_parser((char*)schedule_list, "off_at%64s", tmp_time);
    if(strlen(tmp_time) == 0)
        return -1;
    else
        para->time_list[index].off_at = convert_int_time(tmp_time);

    plog("off_at[%d]\r\n", para->time_list[index].off_at);
    memset(&tmp_time, 0x0, sizeof(tmp_time));
    json_object_parser((char*)schedule_list, "on_at%64s", tmp_time);
    if(strlen(tmp_time) == 0)
        return -1;
    else
        para->time_list[index].on_at = convert_int_time(tmp_time);

    memset(&tmp_time, 0x0, sizeof(tmp_time));
    json_object_parser((char*)schedule_list, "repeat_day%64s", tmp_time);
    if(strlen(tmp_time) == 0)
        return -1;
    else
        para->time_list[index].repeat_day = atoi(tmp_time);

    memset(&tmp_time, 0x0, sizeof(tmp_time));
    json_object_parser((char*)schedule_list, "schedule_id%64s", tmp_time);
    if(strlen(tmp_time) == 0)
        return -1;
    else
        para->time_list[index].schedule_id = atoi(tmp_time);
    return 0;

}

void parser_schedule_list(const char * schedule_list, web_sync_param_t *para)
{
    if(strstr(schedule_list, "[") == NULL)
        return;
    int valid_count = 0;

    int index = json_array_get_length(schedule_list);

    int array_size = sizeof(para->time_list)/sizeof(schedule_time);
    if(index > array_size)
        index = array_size;

    if(index)
    {
        char* list = NULL;
        for(int i = 0; i < index; i++ )
        {	
            list = json_array_get_object_by_idx((char*)schedule_list, i);
            if(list!= NULL)
            {
                if(0==conver_time_list_to_global(valid_count, list, para))
                    valid_count++;
            }
            free(list);
            list= NULL;
        }
    }

    if(valid_count < array_size)
    {
        memset(&para->time_list[valid_count], 0x0,
                sizeof(schedule_time)*(array_size-valid_count));
    }
}

void parse_sub_device_list(const char *sub_device_list)
{
    int num = 0;
    sub_device_info *devices_info = NULL;

    if(!g_p2p_envir.set_sub_device_list_callback)
    {
        plog("warnning:set_sub_device_list_callback is NULL\n");
        return;
    }
    JsonParser json(sub_device_list);

    const char *list= json.GetStringConst("list");

    if(list && strlen(list))
    {
        int index = json_array_get_length(list);

        if(index)
        {
            char* sub = NULL;
            devices_info = (sub_device_info *)malloc(index*sizeof(sub_device_info));
            if(!devices_info) return;

            memset(devices_info, 0, index*sizeof(sub_device_info));

            for(int i = 0; i < index; i++ )
            {	
                sub = json_array_get_object_by_idx(list, i);
                if(sub!= NULL)
                {
                    JsonParser jp(sub);
                    jp.GetStringValue(devices_info[num].physical_id, 
                            sizeof(devices_info[num].physical_id), "physical_id");
                    jp.GetStringValue(devices_info[num].device_name, 
                            sizeof(devices_info[num].device_name),  "device_name");
                    jp.GetStringValue(devices_info[num].local_pwd, 
                            sizeof(devices_info[num].local_pwd),  "local_pwd");
                    jp.GetIntValue(devices_info[num].device_type, "device_type");
                    jp.GetIntValue(devices_info[num].channel_id, "channel_id");
                    jp.GetIntValue(devices_info[num].use_on, "use_on");
                    jp.GetStringValue(devices_info[num].lan_ip, 
                            sizeof(devices_info[num].lan_ip),  "lan_ip");
                    jp.GetIntValue(devices_info[num].buzzer_last, "buzzer_last");
                    jp.GetIntValue(devices_info[num].buzzer_trigger, "buzzer_trigger");
                    jp.GetIntValue(devices_info[num].mode_buzzer, "mode_buzzer");     
                    /* 小温控的温度 */
                    jp.GetIntValue(devices_info[num].temperature, "temperature");   
                    /* 通风孔的开关 */
                    jp.GetIntValue(devices_info[num].vent_switch, "vent_switch");

                    num++;	
                    free(sub);
                }
            }
            g_p2p_envir.set_sub_device_list_callback(num, devices_info);	
            free(devices_info);
        }
        else if(index == 0)
            g_p2p_envir.set_sub_device_list_callback(0, NULL);	
    }

}

void p2p_parse_push_condition(const char* push_condition)
{
    int num = 0;
    p2p_push_condition_t *pc = NULL;

    if(!g_p2p_envir.on_set_push_condition)
    {
        plog("warnning:on_set_push_condition callback is NULL\n");
        return;
    }

    if(push_condition && strlen(push_condition))
    {
        int index = json_array_get_length(push_condition);

        if(index)
        {
            char* sub = NULL;
            pc = (p2p_push_condition_t *)calloc(index, sizeof(p2p_push_condition_t));
            if(!pc) return;

            for(int i = 0; i < index; i++ )
            {	
                sub = json_array_get_object_by_idx(push_condition, i);
                if(sub!= NULL)
                {
                    JsonParser jp(sub);
                    jp.GetStringValue(pc[num].delta, sizeof(pc[num].delta), "delta");
                    jp.GetStringValue(pc[num].push_time, sizeof(pc[num].push_time), "push_time");
                    jp.GetIntValue(pc[num].alarm_interval, "alarm_interval");
                    jp.GetIntValue(pc[num].after_nightview, "after_nightview");
                    jp.GetStringValue(pc[num].width_height_div, sizeof(pc[num].width_height_div), "width_height_div");
                    jp.GetStringValue(pc[num].speed, sizeof(pc[num].speed), "speed");
                    const char* position = jp.GetStringConst("position");
                    if(position)
                    {
                        json_object_parser(position, "x%dy%dw%dh%d", &pc[num].position_x, &pc[num].position_y, &pc[num].position_w, &pc[num].position_h);
                    }
                    jp.GetIntValue(pc[num].push_type, "push_type");
                    jp.GetIntValue(pc[num].picture_count, "picture_count");
                    jp.GetIntValue(pc[num].video_last, "video_last");
                    jp.GetIntValue(pc[num].video_bitrate, "video_bitrate");
                    jp.GetIntValue(pc[num].change_bitrate_interval, "change_bitrate_interval");
                    jp.GetIntValue(pc[num].if_push, "if_push");
                    jp.GetIntValue(pc[num].picture_catch_interval, "picture_catch_interval");
                    jp.GetStringValue(pc[num].movition_last, sizeof(pc[num].movition_last), "movition_last");
                    jp.GetIntValue(pc[num].is_main, "is_main");
                    jp.GetStringValue(pc[num].alarm_interval_span, sizeof(pc[num].alarm_interval_span), "alarm_interval_span");
                    jp.GetIntValue(pc[num].move_length, "move_length");
                    if(pc[num].move_length == 0)
                        pc[num].move_length = 60;
                    num++;	
                    free(sub);
                }
            }
            g_p2p_envir.on_set_push_condition(pc, num);	
            free(pc);
        }
        else if(index == 0)
            g_p2p_envir.on_set_push_condition(NULL, 0);	
    }
}

void p2p_parse_pir_alarm_strategy(const char* as)
{
    JsonParser jp(as); 

    int type = -1;
    jp.GetIntValue(type, "type");

    if(type == 2)
    {
        int interval = 60;
        int m_time = 15;
        int s_time = 5;
        int cloud_m_video_last = 30;
        int cloud_s_video_last = 10;

        jp.GetIntValue(cloud_m_video_last, "cloud_m_video_last");
        jp.GetIntValue(cloud_s_video_last, "cloud_s_video_last");

        jp.GetIntValue(interval, "interval");
        jp.GetIntValue(m_time, "m_time");
        jp.GetIntValue(s_time, "s_time");
        const char* sl = jp.GetStringConst("bs_relation");

        if(!sl || strlen(sl) == 0)
            return;
        int index = json_array_get_length(sl);

        p2p_alarm_stream_level* l = (p2p_alarm_stream_level*)malloc(index*sizeof(p2p_alarm_stream_level));

        memset(l, 0, sizeof(p2p_alarm_stream_level)*index);
        for(int i = 0; i < index; i++ )
        {	
            char* sub = json_array_get_object_by_idx(sl, i);
            if(sub!= NULL)
            {
                JsonParser jf(sub);
                jf.GetIntValue(l[i].bit_rate, "rate");
                jf.GetIntValue(l[i].frame_rate, "frame_rate");
                jf.GetIntValue(l[i].delay, "delayed");
                free(sub);
            }
        }

        if(index > 0 && g_p2p_envir.on_set_pir_alarm_strategy)
        {
            g_p2p_envir.on_set_pir_alarm_strategy(interval, m_time, s_time, cloud_m_video_last, cloud_s_video_last, l, index);
        }
        if(l) free(l);
    }
    else if(type == 3)
    {
        int cloud_m_video_last = 30;
        int cloud_s_video_last = 10;

        jp.GetIntValue(cloud_m_video_last, "cloud_m_video_last");
        jp.GetIntValue(cloud_s_video_last, "cloud_s_video_last");

        const char* sl = jp.GetStringConst("bs_relation");

        if(!sl || strlen(sl) == 0)
            return;
        int index = json_array_get_length(sl);

        p2p_alarm_stream_level* l = (p2p_alarm_stream_level*)malloc(index*sizeof(p2p_alarm_stream_level));

        memset(l, 0, sizeof(p2p_alarm_stream_level)*index);
        for(int i = 0; i < index; i++ )
        {	
            char* sub = json_array_get_object_by_idx(sl, i);
            if(sub!= NULL)
            {
                JsonParser jf(sub);
                jf.GetIntValue(l[i].bit_rate, "rate");
                jf.GetIntValue(l[i].frame_rate, "frame_rate");
                jf.GetIntValue(l[i].delay, "delayed");
                free(sub);
            }
        }

        if(index > 0 && g_p2p_envir.on_set_md_alarm_strategy)
        {
            g_p2p_envir.on_set_md_alarm_strategy(cloud_m_video_last, cloud_s_video_last, l, index);
        }
        if(l) free(l);
    }
}

void parse_preset_list(const char *list)
{	
    if(!g_p2p_envir.set_preset_list_callback)
    {
        plog("warning: set_preset_list_callback is NULL\n");
        return;
    }
    int index = json_array_get_length(list);

    if(index == 0)
    {
        g_p2p_envir.set_preset_list_callback(0, NULL);
        return;
    }
    if(index < 0) return;

    p2p_pt_preset_info_t * preset_info = 
        (p2p_pt_preset_info_t *)malloc(index*sizeof(p2p_pt_preset_info_t));
    if(!preset_info) return;

    int num = 0;

    for(int i = 0; i < index; i++ )
    {	
        char* sub = json_array_get_object_by_idx(list, i);
        if(sub!= NULL)
        {
            JsonParser jp(sub);
            jp.GetStringValue(preset_info[num].preset_name, 
                    sizeof(preset_info[num].preset_name), "preset_name");
            jp.GetStringValue(preset_info[num].trigger_id, 
                    sizeof(preset_info[num].trigger_id), "trigger_id");
            jp.GetIntValue(preset_info[num].preset_on, "preset_on");
            num++;	
            free(sub);
        }
    }
    g_p2p_envir.set_preset_list_callback(num, preset_info);

    free(preset_info);
}
static void p2p_set_sync_data_task(void *para)
{
    web_sync_param_t *sync = (web_sync_param_t*)para;
    p2p_set_sync_paramter(sync);
    free(sync);
}
int p2p_parser_sync_data(char * sync_key, const char * data)
{
    web_sync_param_t *para, get_para;

    para = &get_para;

    p2p_get_sync_paramter(para);
    if(sync_key == NULL || data == NULL)
        return -1;	

    p2p_write_string2file("/tmp/sync_out", data);

    const char *schedule_list = NULL;

    JsonParser jp(data);
    jp.GetStringValue(para->time_zone, sizeof(para->time_zone), SYNC_TIMEZONE);
    jp.GetIntValue(para->time_offset, SYNC_TIMEOFFSET);

    if(strcmp( para->sync_key, sync_key) != 0)
    {		
        strcpy(para->sync_key, sync_key);

        jp.GetIntValue(para->mute, SYNC_MUTE);
        jp.GetIntValue(para->device_on, SYNC_DEVECE_ON);
        jp.GetIntValue(para->device_schedule, SYNC_DEVICE_SCHEDULE);
        jp.GetIntValue(para->cvr_on, SYNC_CVR_ON);
        jp.GetIntValue(para->alarm_interval, SYNC_ALARM_INTERVAL);
        jp.GetIntValue(para->sensitivity, SYNC_SENSITIVITY); 
        jp.GetIntValue(p2p_base()->cvr_timeout_intv, SYNC_CVR_INTERVAL);
        jp.GetIntValue(p2p_base()->cloud_iframe_interval, SYNC_CLOUD_IFRAME_INTERVAL);
        jp.GetIntValue(p2p_base()->cloud_stream_type, SYNC_CLOUD_STREAM_TYPE);
        jp.GetIntValue(para->sound_sensitivity, SYNC_SOUND_SENSITIVITY);
        jp.GetIntValue(para->user_mode, SYNC_USER_MODE);
        jp.GetIntValue(para->device_volume, SYNC_VOLUME);
        jp.GetIntValue(para->statistics_interval, SYNC_STATISTICS_INTERVAL);
        jp.GetIntValue(para->alarm_stream_bitrate, SYNC_ALARM_STREAM_BITRATE);
        jp.GetIntValue(para->seconds_of_microwave_before_md, SYNC_SEC_MW_BEFORE_MD);
        jp.GetIntValue(para->device_backlight, SYNC_BACKLIGHT);
        jp.GetIntValue(para->light_brightness, SYNC_LIGHT_BRIGHTNESS);
        jp.GetIntValue(para->frame_rate, SYNC_FRAME_RATE);

        if(p2p_check_device_ext_ability(EA_MICROWAVE_DETECTION))
            jp.GetIntValue(para->microwave_switch, SYNC_MICROWAVE_SWITCH);
        if(p2p_check_device_ext_ability(EA_NIGHT_SWITCH))
            jp.GetIntValue(para->nightvision_switch, SYNC_NIGHTSWITCH);
        if(p2p_check_device_ext_ability(EA_IMAGE_FLIP))
            jp.GetIntValue(para->imageflip_switch, SYNC_IMAGEFLIP);

        /* 门铃特有的 */
        if(p2p_get_device_type() == DEVICE_TYPE_BELL || p2p_get_device_type() == DEVICE_TYPE_SIMPLE_BELL)
        {
            JsonParser_GetBitfieldValue(para->use_voice_message, jp, SYNC_USE_VOICE_MSG);
            JsonParser_GetBitfieldValue(para->chime,jp,SYNC_CHIME);

            /* 留言列表 */
            const char* voice_message_list = jp.GetStringConst("voice_message_list");
            if(voice_message_list)
            {
                VoiceMessageKeeper kp;
                if(0 == kp.Parse(voice_message_list))
                    para->voice_message_index = kp.GetUseIndex();
            }
        }

        /* lamp */
        jp.GetIntValue(para->lamp.light_switch, SYNC_LIGHT_SWITCH);
        jp.GetIntValue(para->lamp.white_switch, SYNC_WHITE_SWITCH);
        jp.GetIntValue(para->lamp.breath_switch, SYNC_BREATH_SWITCH);

        const char *rgb = jp.GetStringConst(SYNC_RGB);
        if(rgb && strlen(rgb) > 0)
        {
            int r,g,b;
            sscanf(rgb, "%d,%d,%d", &r,&g,&b);

            para->lamp.color_red = (unsigned char)r;
            para->lamp.color_green = (unsigned char)g;
            para->lamp.color_blue = (unsigned char)b;
        }
        const char* sunrise = jp.GetStringConst(SYNC_SUNRISE);
        const char* sunset = jp.GetStringConst(SYNC_SUNSET);
        if(sunrise && sunset && strlen(sunrise) && strlen(sunset))
        {
            int hour = 0;
            int min = 0;
            sscanf(sunrise, "%d:%d", &hour, &min);
            para->lamp.sunrise_hour = hour;
            para->lamp.sunrise_min = min;
            sscanf(sunset, "%d:%d", &hour, &min);
            para->lamp.sunset_hour = hour;
            para->lamp.sunset_min = min;
        }

        /* parser schedule list */

        schedule_list = jp.GetStringConst(SYNC_SCHEDULE_LIST);
        if(schedule_list && strlen(schedule_list) > 0)
        {
            /* old way */
            parser_schedule_list(schedule_list, para);
        }

        /* pareser sub device list */
        if(p2p_check_bit(p2p_get_device_extend_capacity(), EA_SUB_DEVICE))
        {
            const char *sub_device_list = jp.GetStringConst(SYNC_SUB_DEVICES);
            if(sub_device_list && strlen(sub_device_list)>0)
            {
                parse_sub_device_list(sub_device_list);
            }
        }

        /* parser pt preset */
        if(p2p_check_bit(p2p_get_device_extend_capacity(), EA_PT_PRESET))
        {			
            const char *preset_list = jp.GetStringConst(SYNC_PRESET);
            if(preset_list && strlen(preset_list))
            {
                parse_preset_list(preset_list);
            }
        }		
        JsonParser_GetBitfieldValue(para->light_schedule,jp,SYNC_LIGHT_SCHEDULE);
        JsonParser_GetBitfieldValue(para->sound_detetion,jp,SYNC_SOUND_DETECTION);

        const char* ircut_adc = jp.GetStringConst(SYNC_NIGHT_ADC);
        if(ircut_adc && strlen(ircut_adc))
        {
            /* 360~560 */
            sscanf(ircut_adc, "%hi~%hi", &para->ircut_admin, &para->ircut_admax);
        }

        /* thermostat */
        if(p2p_get_device_type() == DEVICE_TYPE_BIG_THERMOSTAST)
        {
            p2p_thermostatinfo_t info;

            memset(&info, '0', sizeof(info));

            info.work_mode = jp.GetIntValue(SYNC_WORK_MODE);
            info.air_switch = jp.GetIntValue(SYNC_AIR_SWITCH);
            info.cool_temp = jp.GetIntValue(SYNC_COOL_TEMP);
            info.heat_temp = jp.GetIntValue(SYNC_HEAT_TEMP);
            info.dry_temp = jp.GetIntValue(SYNC_DRY_TEMP);
            info.ven_temp = jp.GetIntValue(SYNC_VEN_TEMP);
            if(info.work_mode != INVALID_INT)
                p2p_set_thermostatinfo(&info);
        }

        const char* pc = jp.GetStringConst(SYNC_PUSH_CONDITION);
        if(pc) p2p_parse_push_condition(pc);
        jp.GetIntValue(p2p_base()->cover_refresh_time, SYNC_COVER_REFRESH_TIME);
        jp.GetIntValue(p2p_base()->dm_record_time, SYNC_DM_RECORD_TIME);
        jp.GetIntValue(p2p_base()->rtsp_frame_rate, SYNC_RTSP_FRAME_RATE);
        jp.GetIntValue(p2p_base()->rtsp_bit_rate, SYNC_RTSP_BIT_RATE);
        const char* as = jp.GetStringConst(SYNC_ALARM_STRATEGY);
        if(as) p2p_parse_pir_alarm_strategy(as);

        std::map<std::string, config_key_cb_t>::iterator it;

        for(it = g_config_key_callback.begin(); it!=g_config_key_callback.end();it++)
        {
            const char* value = jp.GetStringConst(it->first.c_str());
            if(value)
                it->second(it->first.c_str(), value);
        }
    }

    p2p_handle_para_change(para);
    web_sync_param_t *p = (web_sync_param_t*)malloc(sizeof(web_sync_param_t));
    memcpy(p, para, sizeof(web_sync_param_t));
    ZmdCreateTask( p2p_set_sync_data_task, p, "set_sync_para");
    /* new way */
    if(schedule_list && strlen(schedule_list) > 0)
    {
        ZmdCreateDelayTask(100, p2p_convert_schedule, strdup(schedule_list), "set_sync_para");
    }
    return 0;
}

int p2p_get_sync_paramter(web_sync_param_t* param)
{	
    webserver_get_device(param);	
    return 0;
}
static ZMutex g_web_sync_set_lock;
int p2p_set_sync_paramter(web_sync_param_t* param)
{
    ZMutexLock l(&g_web_sync_set_lock);
    webserver_set_device(param);
    return 0;
}
int p2p_is_restored()
{
    web_sync_param_t para;

    p2p_get_sync_paramter(&para);
    if(strcmp(para.sync_key, "0") == 0)
        return 1;
    return 0;
}
int p2p_get_sync_cvr_on()
{
    web_sync_param_t para;

    p2p_get_sync_paramter(&para);
    return para.cvr_on;
}

void p2p_handle_para_change(web_sync_param_t* net_data)
{
    web_sync_param_t local_data;

    p2p_get_sync_paramter(&local_data);

    if(strcmp(net_data->time_zone, local_data.time_zone) || 
            net_data->time_offset != local_data.time_offset)
    {
        p2p_set_device_timezone(net_data->time_zone, net_data->time_offset);
    }
}
/* 每秒发6次， 总共发60次 */
static void broadcast_timer(void* para)
{
#ifndef _BUILD_FOR_NVR_
    int try_times = (int)para;
    if( try_times-- > 0)
    {
        plogfn();
        CNetServer::getInstance()->BroadcastDeviceInfo();
        ZmdCreateShortTimer(166, broadcast_timer, (void*)try_times );
    }
#endif
}
void p2p_broadcast_device()
{
#ifndef _BUILD_FOR_NVR_
    ZmdCreateShortTimer(0, broadcast_timer, (void*)60 );
#endif
}
#if !defined(_BUILD_FOR_NVR_) && !defined(AMBA) 
//from wifi module
extern WIFI_Connect_Status WIFI_ConnectStatus;
#endif
bool p2p_wifi_connected()
{
    if(g_p2p_envir.use_wired_network)
        return true;

#ifndef _BUILD_FOR_NVR_
    //9 or 15 means wifi connected
#ifdef AMBA
    if(0 == Get_Wifi_Status())
#else
        if(WIFI_ConnectStatus == WPA_COMPLETED || WIFI_ConnectStatus == 15)
#endif
        {
            return true;
        }

    return false;
#else
    return true;
#endif
}

int  p2p_get_playback_list_by_date(int chl, const char* date,  p2p_record_info_t **list, int *num)
{
    p2p_find_playback_file_t* info = NULL;

    *num = 0;

    if(g_p2p_envir.find_playback_list_callback)
        g_p2p_envir.find_playback_list_callback(date, chl, &info, num);

    if(*num == 0) return 0;
    *list = (p2p_record_info_t*)malloc(*num * sizeof(p2p_record_info_t));
    if(!(*list))
    {
        free(info);
        return -1;
    }
    for(int i=0; i<*num; i++)
    {
        //plog("create time[%s], finish time[%s]\n", info[i].create_time, info[i].finish_time);
        (*list)[i].start_time = strdate2sec(date) + p2p_strtime2sec(info[i].create_time);
        (*list)[i].end_time = strdate2sec(date) + p2p_strtime2sec(info[i].finish_time);

        static int rec_type[6] = {1, 4, 3, 1, 0, 0};
        (*list)[i].type = rec_type[info[i].file_type];
        (*list)[i].level =  info[i].alarm_level;
        strcpy((*list)[i].fpath , info[i].file_path);
    }
    free(info);

    return 0;
}

int p2p_get_local_net_info(const char *dev_name, char *ipv4, char *mask, char *mac)
{
    int ret = 0;
    struct ifreq req;
    struct sockaddr_in *host = NULL;

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if ( -1 == sockfd )
    {
        return -1;
    }

    memset(&req, 0, sizeof(struct ifreq));

    strcpy(req.ifr_name, dev_name);
    if ( ioctl(sockfd, SIOCGIFADDR, &req) >= 0 )
    {
        host = (struct sockaddr_in *)&req.ifr_addr;
        strcpy(ipv4, inet_ntoa(host->sin_addr));
    }
    else
    {
        ret = -1;
    }

    memset(&req, 0, sizeof(struct ifreq));
    strcpy(req.ifr_name, dev_name);
    if ( ioctl(sockfd, SIOCGIFNETMASK, &req) >= 0 )
    {
        host = (struct sockaddr_in *)&req.ifr_addr;
        strcpy(mask, inet_ntoa(host->sin_addr));
    }
    else
    {
        ret = -1;
    }

    memset(&req, 0, sizeof(struct ifreq));
    strcpy(req.ifr_name, dev_name);
    if ( ioctl(sockfd, SIOCGIFHWADDR, &req) >= 0 )
    {
        sprintf(
                mac, "%02x:%02x:%02x:%02x:%02x:%02x",
                (unsigned char)req.ifr_hwaddr.sa_data[0],
                (unsigned char)req.ifr_hwaddr.sa_data[1],
                (unsigned char)req.ifr_hwaddr.sa_data[2],
                (unsigned char)req.ifr_hwaddr.sa_data[3],
                (unsigned char)req.ifr_hwaddr.sa_data[4],
                (unsigned char)req.ifr_hwaddr.sa_data[5]
               );
    }
    else
    {
        ret = -1;
    }

    if ( sockfd != -1 )
    {
        close(sockfd);
    }
    return ret; 
} 

int p2p_get_gateway_info(const char *dev_name, char *gw_ip, char *gw_mac)
{
    int is_done = 0;	
#ifdef ANDROID_API_LEVEL_LOW
	char line[2048] = {0};
#else
    std::vector<char> v(2048, '\0');
    char *line = &v[0];
#endif
    size_t n=2048;

    /* get gate ip */
    FILE *file = p2p_fopen("/proc/net/route", "r");

    if(file)
    {
        char iface[32]={0};
        char dest[32]={0};
        char gw[32]={0};
        char *endptr = NULL;

        if(p2p_get_filesize(file) > 1024)
        {
            fclose(file);
            return -1;
        }

        do
        {
#ifdef ANDROID_API_LEVEL_LOW
			fgets(line, n, file);
#else
            getline(&line, &n, file);
#endif

            sscanf(line, "%s %s %s", iface, dest, gw);

            /* default getway ? */
            if(strcmp(iface, dev_name) == 0 && strtol(dest, &endptr, 16)==0)
            {
                struct in_addr addr;

                addr.s_addr = strtol(gw, &endptr, 16);
                strcpy(gw_ip, inet_ntoa(addr));
                is_done = 1;
                break;
            }
        }while(!feof(file));

        fclose(file);
        if(!is_done) 
        {
            plog("get getway ip failed!\n");
            return -1;
        }
    }
    else
    {
        plog("open route file failed!\n");
        return -1;
    }

    /* get getway mac */
    file = p2p_fopen("/proc/net/arp", "r");

    if(file)
    {
        char ip_addr[32]={0};
        char hw_types[32]={0};
        char flags[32]={0};
        char mac[32]={0};

        if(p2p_get_filesize(file) > 1024)
        {
            fclose(file);
            return -1;
        }
        is_done = 0;
        do
        {
#ifdef ANDROID_API_LEVEL_LOW
			fgets(line, n, file);
#else
            getline(&line, &n, file);
#endif

            sscanf(line, "%s %s %s %s", ip_addr, hw_types, flags, mac);

            /* getway ip ? */
            if(strcmp(ip_addr, gw_ip) == 0)
            {
                strcpy(gw_mac, mac);
                is_done = 1;
                break;
            }
        }while(!feof(file));

        fclose(file);
        if(!is_done) 
        {
            plog("get getway mac failed!\n");
            return -1;
        }
    }
    else
    {
        plog("open arp file failed!\n");
        return -1;
    }

    return 0;
}
const char *p2p_get_network_interface()
{
    return g_p2p_envir.network_interface;
}

int p2p_network_ok()
{
    char ip[32], mask[32], mac[32];

    if(g_p2p_envir.use_wired_network)
    {
        return !p2p_get_local_net_info(p2p_get_network_interface(), ip, mask, mac);
    }
    else
    {
        return p2p_wifi_connected() && 
            !p2p_get_local_net_info(p2p_get_network_interface(), ip, mask, mac);
    }
}

typedef enum WIFI_Connect_Time 
{ 
    START_CONNECT=0, 
    SD_FIRST_LINK , 
    RECONNECT_LINK 

}WIFI_Connect_Time;
#ifndef _BUILD_FOR_NVR_
/* from libwifi.a */
extern WIFI_Connect_Time WIFI_connectTime; 
#else
static WIFI_Connect_Time WIFI_connectTime = START_CONNECT; 
#endif

int p2p_get_smartlink_mark()
{
    return WIFI_connectTime == SD_FIRST_LINK;
}

void p2p_clear_smartlink_mark()
{
    plogfn();
    WIFI_connectTime = START_CONNECT;
}

void p2p_get_wifi_info(char* ssid, char* pwd)
{
    NETWORK_PARA netWork;

    PubGetSysParameter(SYSNET_SET, &netWork);
#ifndef _BUILD_FOR_NVR_
    strcpy(ssid, netWork.m_WifiConfig.LoginWifiDev.RouteDeviceName);
    strcpy(pwd,  netWork.m_WifiConfig.LoginWifiDev.Passwd);
#else
    strcpy(ssid, "");
    strcpy(pwd, "");
#endif
    //plog("ssid:[%s], pwd:[%s]\n", ssid, pwd);
}

void p2p_md5_string(const char* str, char* out)
{
    char tmp[32] = "/tmp/XXXXXX";

    int fd = mkstemp(tmp);

    write(fd, str, strlen(str));	

    FILE *file = fdopen(fd, "rb");

    fseek(file, 0, SEEK_SET);

    if(file)
    {
        MD5VAL v = md5File(file);

        sprintf(out, "%08X%08X%08X%08X", ntohl(v.a),ntohl(v.b),ntohl(v.c),ntohl(v.d));
        fclose(file);
    }
    unlink(tmp);
}

int p2p_is_device_on()
{
#ifndef _BUILD_FOR_NVR_
    if(p2p_check_device_ext_ability(EA_DEVICE_SCHEDULE))
        return GetWebScheduleSwitch(0);
#endif
    return 1;
}

int p2p_check_device_ext_ability(extend_ability_name_e ab)
{
    return p2p_check_bit(p2p_get_device_extend_capacity(), ab);
}
int p2p_check_device_new_ability(new_ability_name_e ab)
{
    return p2p_get_device_supply_capacity() & ((unsigned long long)1<<(int)ab);
}
#ifndef AMBA
extern char get_router_mac[32];
#endif
int p2p_get_wifi_router_mac(char *mac)
{
#ifndef _BUILD_FOR_NVR_
    if(!p2p_wifi_connected()) return -1;
#ifdef AMBA
    GetWifiRouterMac(mac);
#else
    strcpy(mac, get_router_mac);
#endif
    //toupper
    char *p = mac;
    do{
        *p=toupper(*p);
    }while(*(++p));
    return 0;
#else
    return -1;
#endif
}
void p2p_reset_device()
{
#ifndef _BUILD_FOR_NVR_
#ifdef AMBA
    RestoreDefault();
#else
    RestoreDefault(NULL);
#endif
#else
    RestoreFactory("p2p_reset_device");
#endif
}

void p2p_reboot_device()
{
    RebootSystem();
}

int  p2p_add_hi_head_to_g711(const char* in_file, const char* out_file)
{
    char hi_head[] = { 0x00, 0x01, 0x50, 0x00 };

    char buffer[164];

    const int frm_size = 160;

    memcpy(buffer, hi_head, 4);

    char* read_pos = buffer + 4;
    char* write_pos = buffer;

    int fsize = get_filesize(in_file);

    if (fsize <= 0) return -1;

    fsize = (fsize / frm_size) * frm_size;

    FILE* in = p2p_fopen(in_file, "rb");

    FILE* out = p2p_fopen(out_file, "wb");

    while (in && out && !feof(in)) {
        int ret = fread(read_pos, 1, frm_size, in);

        if (ret < frm_size) {
            fclose(in);
            fclose(out);
            break;
        }
        fsize -= frm_size;
        fwrite(write_pos, 1, frm_size + 4, out);
    }
    if (fsize < 160) return 0;

    return -1;
}

void p2p_handle_answer_call()
{
    if(g_p2p_envir.answer_call_callback) 
        g_p2p_envir.answer_call_callback();
}

void p2p_play_voice(const char* voice_message_id)
{
    if(voice_message_id)
    {		
        VoiceMessageKeeper kp;

        const char* voice_path = kp.GetFilePath(voice_message_id);

        plog("play voice message [%s]\n", voice_message_id);

        if(g_p2p_envir.play_audio_file_callback && voice_path)
        {
            plog("play voice file [%s]\n", voice_path);
            g_p2p_envir.play_audio_file_callback(voice_path);
        }
    }
}
static void on_refresh_cover_imp(void*)
{
    for(int i=0; i< p2p_get_device_chlnum();i++)
    {
        p2p_send_cover_pic(i);
    }
}
static void on_refresh_device(void*)
{
    if(g_p2p_envir.on_refresh_device_callback)
    {
        int ret = 0;
        if((ret = g_p2p_envir.on_refresh_device_callback()) > 0)
            ZmdCreateTimer(ret, on_refresh_device, NULL);
    }
}
void p2p_on_refresh_cover()
{
    //	plogfn();
    ZmdCreateTimer(0, on_refresh_cover_imp, NULL);
    /*临时给pivot刷新温湿度用*/
    if(g_p2p_envir.on_refresh_device_callback)
        ZmdCreateTimer(0, on_refresh_device, NULL);
}

void p2p_on_hang_up()
{

}

void p2p_on_refuse_answer()
{

}

void p2p_handle_timing(int timestamp)
{	
    if(timestamp > 0) 
    {
        p2p_base()->server_timestamp_base_uptime = timestamp - p2p_get_uptime();
        if(g_p2p_envir.timing_callback && p2p_is_ntp_enable())
        {
            g_p2p_envir.timing_callback(timestamp);
        }
        else
        {
            plog("\n");
            plog("timing_callback not set or ntp not enable !!!\n");
            plog("timing_callback not set or ntp not enable !!!\n");
            plog("timing_callback not set or ntp not enable !!!\n");
            plog("\n");
        }
    }
}

int p2p_get_server_timestamp_imp()
{
    if(p2p_base()->server_timestamp_base_uptime > 0)
    {
        return p2p_get_uptime() + p2p_base()->server_timestamp_base_uptime;
    }
    else
    {
        plog("get failed!\n");
        return -1;
    }
}

int p2p_is_ntp_enable()
{
    return NtpIfEnable();
}

#ifndef _BUILD_FOR_NVR_
extern int  HaveRecordFile(struct tm time);
#else
extern int  HaveRecordFile(int channel, struct tm);
#endif
int p2p_if_have_record_file_by_date(struct tm tm_date, int channel)
{
    tm_date.tm_year -= 100;
    tm_date.tm_mon += 1;
#ifdef PLAYBACK
#ifndef _BUILD_FOR_NVR_
    return HaveRecordFile(tm_date);
#else
    return HaveRecordFile(channel, tm_date);
#endif
#else
    plog("\n\n error : this build is not support remote playback!!!\n\n");
    return 0;
#endif
}

static int stream_request_status[32][3];

void  p2p_on_video_request(int stream_type, int channel)
{
    plog("stream_type:%d, channel:%d\n", stream_type, channel);

    if(stream_type < 3)
        stream_request_status[channel][stream_type] = 1;
}

void p2p_on_video_release(int stream_type, int channel)
{
    plog("stream_type:%d, channel:%d\n", stream_type, channel);
    if(stream_type < 3)
        stream_request_status[channel][stream_type] = 0;
}

int  p2p_get_request_video_status(int stream_type, int channel)
{
    if(channel < 32 && stream_type < 3)
        return stream_request_status[channel][stream_type];
    else
        return 0;
}

const char* p2p_get_ap_id(char** ap_usrid, int *number)
{
    const char* apfile = "/tmp/ap_file";
    if(access(apfile, R_OK) == 0)
    {
        char apid[256] = {0};

        FILE *file = fopen(apfile, "r");
        if(file)
        {
            *number = 0;
            fscanf(file, "%[^:]:%d", apid, number);
            fclose(file);
        }
        unlink(apfile);

        if(strlen(apid))
        {
            if(apid[strlen(apid)-1] == '\n')
                apid[strlen(apid)-1] = '\0';
            if(apid[strlen(apid)-1] == '\r')
                apid[strlen(apid)-1] = '\0';
            *ap_usrid = strdup(apid);
        }
        plog("get sap info[%s : %d]\n", apid, *number);
    }
    else
    {
        plog("%s is not reable\n", apfile);
    }
    return *ap_usrid;
}

void p2p_on_device_online(int status)
{
    if(g_p2p_envir.on_device_online_callback)
        g_p2p_envir.on_device_online_callback(status);
}

static void devlogin_task(void*)
{
    if(0 != web_login(p2p_base(), 10))
    {
        sleep(p2p_get_rand(1,5));
        web_login(p2p_base(), 10);
    }
}
int p2p_set_ap_mode(int mode , int device_count, const char* extra_para)
{
    if( mode == 99 )
    {
        return 0;
    }
    else if( mode == 98 )
    {
        ZmdCreateTimer(0, devlogin_task, NULL);
        return 0;
    }
    if(g_p2p_envir.on_ap_mode_callback)
        g_p2p_envir.on_ap_mode_callback(mode, device_count, extra_para);
    return 0;
}

int p2p_get_cvr_interval()
{
    return p2p_base()->cvr_timeout_intv;
}

int p2p_write_upgrade_log()
{
    return 0;
    /*
     * write app version before upgrade
     * */
    char fpath[128];

    strcpy(fpath, p2p_get_config_dir());
    if(strlen(fpath) > 0 && fpath[strlen(fpath)-1] != '/')
        fpath[strlen(fpath)] = '/';
    strcat(fpath, "p2p_upgrade_log");
    FILE *file = p2p_fopen(fpath, "wb");
    if(file)
    {
        fprintf(file, "%s", g_p2p_envir.app_version);
        fclose(file);
    }
    return 0;
}

int p2p_is_boot_after_upgrade()
{
    char fpath[128];
    char version[32];

    strcpy(fpath, p2p_get_config_dir());
    if(strlen(fpath) > 0 && fpath[strlen(fpath)-1] != '/')
        fpath[strlen(fpath)] = '/';
    strcat(fpath, "p2p_upgrade_log");
    FILE *file = p2p_fopen(fpath, "rb");
    if(file)
    {
        fscanf(file, "%s", version);
        fclose(file);
        unlink(fpath);
        plog("get upgrade log %s\n", version);
        if(strcmp(version, g_p2p_envir.app_version)!=0)
            return 1;
    }
    return 0;
}

int p2p_upload_upgrade_success()
{
    char versions[128];

    return 0;
    p2p_get_device_versions(versions);

    for(int i=0;i<3;i++)
    {
        if(0 == web_set_upgrade_state(0, versions))
            break;
        sleep(3);
    }
    return 0;
}

int p2p_set_buzzer(int oper)
{
    if(g_p2p_envir.on_test_buzzer)
    {
        g_p2p_envir.on_test_buzzer(oper);
        return 0;
    }
    return -1;
}

int p2p_md_region_ctrl(int op_type, p2p_md_region_t* reg)
{
    if(g_p2p_envir.on_md_region_ctrl)
    {
        g_p2p_envir.on_md_region_ctrl(op_type, reg); 
        return 0;
    }
    return -1;
}

int p2p_set_thermostatinfo(p2p_thermostatinfo_t *info)
{
    if(g_p2p_envir.on_thermostat_set)
    {
        g_p2p_envir.on_thermostat_set(info);
        return 0;
    }
    return -1;
}
int p2p_watering_ctrl(p2p_watering_ctrl_t *wc)
{
    if(g_p2p_envir.on_watering_ctrl)
    {
        g_p2p_envir.on_watering_ctrl(wc);
        return 0;
    }
    return -1;
}

int p2p_get_curtain_stat(int *curtain_status, int *screen_status)
{
    if(g_p2p_envir.on_curtain_get_state)
    {
        g_p2p_envir.on_curtain_get_state(curtain_status, screen_status);
        return 0;
    }
    return -1;
}

int p2p_curtain_ctrl(int op_type, int is_curtain)
{
    if(g_p2p_envir.on_curtain_ctrl)
    {
        g_p2p_envir.on_curtain_ctrl(op_type, is_curtain);
        return 0;
    }
    return -1;
}

int p2p_preset_ctrl(int op_type, const char* physical_id, const char* preset_name, const char* new_name, char** image_url)
{
    if(g_p2p_envir.on_preset_ctrl)
    {
        return g_p2p_envir.on_preset_ctrl(op_type, physical_id, preset_name, new_name, image_url);
    }
    return -1;
}
int p2p_lock_ctrl(int type, const char* key_info)
{
    if(g_p2p_envir.on_unlocking)
    {
        return g_p2p_envir.on_unlocking(type, key_info);
    }
    return -1;
}
