#include <time.h>
#include <assert.h>
#include <map>

#include "plog.h"
#include "p2p_interface.h"
#include "device_operation.h"
#include "web.h"
#include "p2p_streamer.h"
#include "zmdtimer.h"
#include "zmd_task.h"
#include "p2p_access_client.h"

#define ONLINE_CHECK 	\
	if(!p2p_is_device_online()) \
	{\
		plog("device is offline now!\n");\
		return P2P_ERR_OFFLINE;\
	}
struct report_para_t
{
    void* para;
    int num;
};

std::map<std::string, config_key_cb_t> g_config_key_callback;

int p2p_report_value(p2p_report_value_t * v, int num)
{
	ONLINE_CHECK;
	return web_report_value(v, num);
}
int p2p_report_battery(p2p_report_battery_t * v, int num)
{
	ONLINE_CHECK;
	return web_report_battery(v, num);
}

static void web_report_battery_task(void *para)
{
    report_para_t * sync = (report_para_t*)para;
    for(int i=0; i< 3; i++)
    {
       if(0 == web_report_battery((p2p_report_battery_t*)sync->para, sync->num))
           break;
        sleep(3);
    }
    free(sync->para);
    free(sync);
}

int p2p_report_battery_a(p2p_report_battery_t * v, int num)
{
	ONLINE_CHECK;
    report_para_t * sync = p2p_malloc(report_para_t);
    p2p_report_battery_t *para = p2p_malloc_n(p2p_report_battery_t, num);
    memcpy(para, v, sizeof(p2p_report_battery_t)*num);
    sync->para = para;
    sync->num = num;
    return ZmdCreateTask(web_report_battery_task, sync, "web_report_task");
}
bool p2p_is_online()
{
	return p2p_is_device_online();
}
/* ¼æÈÝ¾É½Ó¿Ú */
bool is_online()
{
	return p2p_is_device_online();
}

int p2p_upload_sub_device_list(int num, const sub_device_info *sd_list)
{
	ONLINE_CHECK;
	
	return web_upload_sub_device_list(num, sd_list);
}

int p2p_upload_pt_preset(int flag, const char* trigger_id,
	const char* preset_name, int  preset_on, 
	const char* image_name, const char* original_name,
	char** image_url)
{
	ONLINE_CHECK;

	return web_upload_pt_preset(flag, trigger_id, preset_name,
	 preset_on, image_name, original_name, image_url);
}
extern int p2p_main();
int p2p_init(p2p_init_info_t* info, unsigned int info_len)
{	
    openlog("<P2P>", LOG_CONS, LOG_USER);
	
	plog("info_len:[%d]\n", info_len);

	//P2P_ASSERT(sizeof(p2p_init_info_t) == info_len, -1);
	while(sizeof(p2p_init_info_t) != info_len)
	{
		plog("p2p_interface.h do not match!!!\n");
		sleep(1);
	}

	while(sizeof(p2p_broadcast_alarm_t) != 12)
	{
		plog("sizeof p2p_broadcast_alarm_t do not equal 12!!!\n");
		sleep(1);
	}
	
#ifndef PLAYBACK
    while(p2p_check_bit(info->device_extend_capacity, EA_REMOTE_PLAYBACK))
    {
        plog("\n\n error : this build is not support remote playback!!!\n\n");
        sleep(1);
    }
#endif
	p2p_set_init_info(info, info_len);
	
	p2p_main();
	return 0;
}

int p2p_get_server_timestamp()
{
	ONLINE_CHECK;
	
	return p2p_get_server_timestamp_imp();
}

int p2p_if_somebody_watching_video()
{
    int status = 0;
    int chlnum = 0 ;
    
    if((chlnum = p2p_get_device_chlnum()) > 32)
        chlnum = 32;

    for(int chl = 0; chl < chlnum; chl++)
    {
        for(int stream_type = 0; stream_type < 3; stream_type++)
        {
            status += p2p_get_request_video_status(stream_type, chl);
        }
    }
    return status;
}

int p2p_report_adding_devices(adding_device_info *ad, int num)
{
	ONLINE_CHECK;
    return web_report_adding_devices(ad, num);
}

struct sync_para_t
{
    char* name;
    char* value;
};
struct sync_para2_t
{
    p2p_sync_value_t *values;
    int values_count;
};

static void web_sync_task_1(void *para)
{
    sync_para_t * sync = (sync_para_t*)para;
    for(int i=0; i< 3; i++)
    {
       if(0 ==  web_sync_value(sync->name, sync->value))
           break;
        sleep(3);
    }
    free(sync->name);
    free(sync->value);
    free(sync);
}
static void web_sync_task_2(void *para)
{
    sync_para2_t * sync = (sync_para2_t*)para;
    int i;
    for(i=0; i< 3; i++)
    {
       if(0 ==  web_sync_values(sync->values, sync->values_count))
           break;
        sleep(3);
    }
    free(sync->values);
    free(sync);
}
int p2p_sync_value(const char* name, const char* value)
{
	ONLINE_CHECK;
    sync_para_t *para = p2p_malloc(sync_para_t);
    para->name = strdup(name);
    para->value = strdup(value);
    return ZmdCreateTask(web_sync_task_1, para, "web_sync_task");
}
int p2p_sync_values(const p2p_sync_value_t* values, int values_count)
{
	ONLINE_CHECK;
    sync_para2_t *para = p2p_malloc(sync_para2_t);
    para->values = p2p_malloc_n(p2p_sync_value_t, values_count);
    memcpy(para->values, values, sizeof(p2p_sync_value_t)*values_count);
    para->values_count = values_count;
    return ZmdCreateTask(web_sync_task_2, para, "web_sync_task");
}
int p2p_sync_values_block(const p2p_sync_value_t* values, int values_count)
{
	ONLINE_CHECK;
    return web_sync_values(values, values_count);
}
extern int p2p_update_flag;
int p2p_if_upgrading()
{
	return p2p_update_flag;
}

int p2p_get_sleep_server_info(sleep_server_info *info)
{
	ONLINE_CHECK;
	return web_rest_rpt(info);
}

int p2p_request_offline()
{
    ONLINE_CHECK;
    return p2p_access_client_stop();
}

int p2p_get_timezonelist(p2p_timezonelist_t *list, int *num)
{
    ONLINE_CHECK;
    return web_get_timezonelist(list, num);
}

int p2p_report_thermostat_stat(p2p_thermostat_stat_t * st)
{
    ONLINE_CHECK;
    return web_report_thermostat_stat(st);
}

int p2p_report_waterholeinfo(const char* physical_id, int hole_id, int watering)
{
    ONLINE_CHECK;
    return web_report_waterholeinfo(physical_id, hole_id, watering);
}

int p2p_report_lockinfo(const char* physical_id, int locked)
{
    ONLINE_CHECK;
    return web_report_lockinfo(physical_id, locked);
}
int p2p_lpr_rpt(const char* pics[], int pic_num)
{
    ONLINE_CHECK;
    return web_lpr_rpt(pics, pic_num);
}

int p2p_face_rpt(const char* pic_path, int face_num, p2p_face_pos_t* pos)
{
    ONLINE_CHECK;
    return web_face_rpt(pic_path, face_num, pos);
}

int p2p_get_voice_file(const char* user_id, char *fpath)
{
    ONLINE_CHECK;
    return web_get_voice_file(user_id, fpath);
}

int p2p_register_config_key_callback(const char* key, config_key_cb_t cb)
{
    g_config_key_callback[key]=cb;
    return 0;
}

#undef ONLINE_CHECK

