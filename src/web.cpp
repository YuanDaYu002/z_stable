#include <curl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "p2pconfig.h"
#include "zjson.h"
#include "web.h"
#include "plog.h"
#include "device_operation.h"
#include <sstream>
#include <vector>
#include "web_task.h"
#include "p2p_interface.h"
#include "p2p_def.h"
#include "zmutex.h"

extern p2p_init_info_t g_p2p_envir;

int web_check_result(const char* json)
{
    char result[32]={0};

    if(!json) return -1;

    if(!json_is_json(json)) return -1;

    json_object_parser(json, "result%32s", result);

    if(!strcmp("0", result) || !strcmp("ok", result)) return 0;

    //"device is not exist", we assume this as success
    if(!strcmp("100100005", result)) return 0;

    //"device does not exist", we assume this as success
    if(!strcmp("100300647", result)) return 0;

    //"not bound by any user", we assume this as success
    if(!strcmp("100300618", result)) return 0;

    //"user id is error", we assume this as success
    if(!strcmp("100100295", result)) return 0;

    //"sub device is bund already", we assume this as success
    if(!strcmp("100100057", result)) return 0;

    //{"result":"1002","error":"tokenid is invalid"}
    if(!strcmp("1002", result))
    {
        p2p_base()->token_valid = 0;
        web_login(p2p_base(), 10);
    }	
    return -1;
}

int web_init()
{
    curl_global_init(CURL_GLOBAL_ALL);
    return 0;
}

//use buf->datalen to check if success, 
//if buf->datalen>0, means recv someting
static void web_get(const char* url, std::string& result, int connect_timeout=10)
{
    WebTask task;

    task.SetUrl(url);
    task.SetConnectTimeout(connect_timeout);
    task.DoGetString();
    if(task.WaitTaskDone() == 0)
    {
        result = task.GetResultString();
    }
}

const char * web_get_tokenid()
{
    return p2p_base()->token;
}

static ZMutex g_devlogin_lock;
//token will reflesh when login success
int web_login(p2p_base_t *base, int timeout)
{
    char url[1024];
    const char *body = NULL;
    std::string buf;

    const char* usr = base->device_id;
    char* aes_key = base->access_aes_key;
    char* aes_key_id = base->aes_key_id;
    char* token = base->token;
    int &token_valid = base->token_valid;

    strcpy(url, WEB_LOGIN_URL);
    strcat(url, "?physical_id=");
    strcat(url, usr);

    plog("login url:%s\n", url);

    web_get(url, buf, timeout);

    if(buf.size() == 0) 
        return -1;

    body = buf.c_str();
    plog("recv reply:%s\n", body);

    if(!json_is_json(body)) return -1;

    JsonParser jp(body);

    const char* result = jp.GetStringConst("result");

    if(result && !strcmp("ok", result))
    {
        int timestamp = 0;

        ZMutexLock l(&g_devlogin_lock);
        //timing must first for using srand(time(NULL))
        jp.GetIntValue(timestamp, "timestamp");
        if(timestamp)
            p2p_handle_timing(timestamp);
        jp.GetStringValue(token, 64, "addition");
        token_valid = 1;
        set_dev_access_servers(jp.GetStringConst("dev_access_addr"));
        set_devconn_servers(jp.GetStringConst("dev_conn_addr"));
        set_alerts_servers(jp.GetStringConst("alerts_addr"));
        set_file_servers(jp.GetStringConst("file_server_addr"));
        set_devmng_servers(jp.GetStringConst("dev_mng_addr"));
        jp.GetStringValue(aes_key, 64, "encrypt_key");
        jp.GetStringValue(aes_key_id, 64, "encrypt_key_id");
        plog_str(token);
        return 0;
    }
    plog("web login failed!!!\n");
    return -1;
}

int web_report_info(const char* usrid, const char* tokenid, int timeout)
{
    WebTask task;
    char* baseinfo_buf = NULL;
    char *config_buf = NULL;
    char versions[64]="";
    static char *ap_usrid = NULL;
    static int  ap_number = 0;

    std::string server = get_devmng_server();
    std::string url = server+std::string(WEB_REPORT_URI);
	task.SetUrl(url.c_str());

    p2p_get_device_versions(versions);	

    /* build baseinfo */
    JSON_OBJECT base_json = json_build_new();

    if(base_json)
    {
        json_build_add_string(base_json, "tokenid", tokenid);
        json_build_add_string(base_json, "physical_id", usrid);
        json_build_add_string(base_json, "device_type", p2p_itoa(p2p_get_device_type()));
        //for smartlink
        if(p2p_get_smartlink_mark())
        {
            char ssid[128]={0};
            char pwd[128]={0};
            char cmp[256];
            char md5_str[33]={0};

            p2p_get_wifi_info(ssid,pwd);

            strcpy(cmp, ssid);
            strcat(cmp, pwd);
            p2p_md5_string(cmp, md5_str);
            json_build_add_string(base_json, "smartlink_task", md5_str);
        }
        //for soft ap
        if(ap_usrid || p2p_get_ap_id(&ap_usrid, &ap_number))
        {
            json_build_add_string(base_json, "adding_userid", ap_usrid);
        }

        char wifi_router_mac[32] = {0};
        if(p2p_get_wifi_router_mac(wifi_router_mac) == 0 && strlen(wifi_router_mac))
            json_build_add_string(base_json, "gateway_mac", wifi_router_mac);

        baseinfo_buf = json_build_end(base_json);
    }

    /* build confinfo */	
    JSON_OBJECT conf_json = json_build_new();

    if(conf_json)
    {
        json_build_add_string(conf_json, "device_channel", p2p_itoa(p2p_get_device_chlnum()));
        json_build_add_string(conf_json, "device_ionum", p2p_itoa(p2p_get_device_io_chlnum()));
        json_build_add_string(conf_json, "device_model", p2p_get_device_name());
        json_build_add_string(conf_json, "device_version", versions);
        json_build_add_string(conf_json, "device_capacity", p2p_utoa(p2p_get_device_capacity()));
        json_build_add_string(conf_json, "device_extend_capacity", p2p_utoa(p2p_get_device_extend_capacity()));
		json_build_add_string(conf_json, "device_supply_capacity", p2p_llutoa(g_p2p_envir.device_supply_capacity));

        //for pivot
        if(p2p_get_device_type() == 7)
        {
            json_build_add_string(conf_json, "temperature_channel", "1");
            json_build_add_string(conf_json, "humidity_channel", "1");
        }
#ifndef _BUILD_FOR_NVR_		
        /* 分辨率 */
        JSON_OBJECT resolution = json_build_new();
        json_build_add_string(resolution, "HD", g_p2p_envir.high_resolution);
        json_build_add_string(resolution, "SD", g_p2p_envir.secondary_resolution);
        json_build_add_string(resolution, "LD", g_p2p_envir.low_resolution);
        char *resolution_str = json_build_end(resolution);
        json_build_add_string(conf_json,"resolution", resolution_str);
        free(resolution_str);
#endif
        if(strlen(g_p2p_envir.aes_key))
            json_build_add_string(conf_json, "aes_key", g_p2p_envir.aes_key);
		if (p2p_get_device_type() == 3 && strlen(g_p2p_envir.module_id_915))
			json_build_add_string(conf_json, "module_id_915", g_p2p_envir.module_id_915);

        config_buf = json_build_end(conf_json);
    }

    task.AddPostString("baseinfo", baseinfo_buf);
    task.AddPostString("confinfo", config_buf);
    task.DoGetString();

    if(baseinfo_buf)
        free(baseinfo_buf);
    if(config_buf)
        free(config_buf);

    if (0 != task.WaitTaskDone())
    {
        plog("web report info failed\r\n"); 
        mark_invalid_server(server.c_str());
        return -1;
    }

    plog("recv:%s", task.GetResultString());
    if(!json_is_json(task.GetResultString()))
        mark_invalid_server(server.c_str());
    if(web_check_result(task.GetResultString()) == 0)
    {
        if(ap_usrid)
        {
            free(ap_usrid);
            ap_usrid = NULL;
        }
        return 0;
    }
    else
        return -1;
}

int web_report_upnp(const char *tokenid, 
        int upnp_video_port,
        int local_video_port,
        const char* dev_mac,
        const char* dev_ip,
        const char* dev_mask,
        const char* gw_mac,
        const char* gw_ip,
        const char* ssid )
{
    WebTask task;

    std::string server = get_devmng_server();
    std::string url = server+std::string(WEB_REPORT_UPNP_URI);
	task.SetUrl(url.c_str());
    task.AddPostString("physical_id", p2p_get_deviceid());
    task.AddPostString("upnp_video_port", p2p_itoa(upnp_video_port));
    task.AddPostString("local_video_port", p2p_itoa(local_video_port));
    task.AddPostString("tokenid", tokenid);
    task.AddPostString("device_mac", dev_mac);
    task.AddPostString("local_ip", dev_ip);
    task.AddPostString("net_mask", dev_mask);
    task.AddPostString("gateway_ip", gw_ip);
    task.AddPostString("gateway_mac", gw_mac);
    if(ssid && strlen(ssid)>0)
        task.AddPostString("ssid", ssid);
    task.DoGetString();
    if (0 != task.WaitTaskDone())
    {
        plog("web report upnp failed\n"); 
        mark_invalid_server(server.c_str());
        return -1;
    }
    if(!json_is_json(task.GetResultString()))
        mark_invalid_server(server.c_str());
    plog("recv:%s\n", task.GetResultString());
    return web_check_result(task.GetResultString());
}

int web_sync_paramter()
{
    char *baseinfo = NULL;		
    web_sync_param_t get_para;

    P2P_ASSERT(sizeof(web_sync_param_t) == 360, -1);

    p2p_get_sync_paramter(&get_para);

    plogfn();

    /* build baseinfo */
    JSON_OBJECT base_json = json_build_new();

    if(base_json)
    {
        json_build_add_string(base_json, "tokenid", web_get_tokenid());
        json_build_add_string(base_json, "physical_id", p2p_get_deviceid());
        json_build_add_string(base_json, "device_type", p2p_itoa(p2p_get_device_type()));
        json_build_add_string(base_json, "synckey", get_para.sync_key);
        baseinfo = json_build_end(base_json);
    }

    char* syncinfo = NULL;
#if 1
    if(strcmp(get_para.sync_key, "0") == 0 )
    {
        JSON_OBJECT json = json_build_new();

        json_build_add_string(json, SYNC_TIMEZONE, get_para.time_zone);			

        if(p2p_check_device_ext_ability(EA_ALARM_INTERVAL))
        {
            json_build_add_string(json, SYNC_ALARM_INTERVAL, p2p_itoa(get_para.alarm_interval));
            //json_build_add_string(json, SYNC_SENSITIVITY, p2p_itoa(get_para.sensitivity));
        }
        if(p2p_check_device_ext_ability(EA_MUTE))
            json_build_add_string(json, SYNC_MUTE, p2p_utoa(get_para.mute));
        if(p2p_check_device_ext_ability(EA_DEVICE_SCHEDULE))
        {
            json_build_add_string(json, SYNC_DEVECE_ON, p2p_utoa(get_para.device_on));
            json_build_add_string(json, SYNC_DEVICE_SCHEDULE, p2p_utoa(get_para.device_schedule));
        }
        if(p2p_check_device_ext_ability(EA_MICROWAVE_DETECTION))
            json_build_add_string(json, SYNC_MICROWAVE_SWITCH, p2p_utoa(get_para.microwave_switch));
        if(p2p_check_device_ext_ability(EA_NIGHT_SWITCH))
            json_build_add_string(json, SYNC_NIGHTSWITCH, p2p_utoa(get_para.nightvision_switch));
        if(p2p_check_device_ext_ability(EA_IMAGE_FLIP))
            json_build_add_string(json, SYNC_IMAGEFLIP, p2p_utoa(get_para.imageflip_switch));

        /* 门铃特有的 */
        if(p2p_get_device_type() == DEVICE_TYPE_BELL)
        {			
            json_build_add_string(json, SYNC_USE_VOICE_MSG, p2p_utoa(get_para.use_voice_message));
            json_build_add_string(json, SYNC_CHIME,p2p_utoa(get_para.chime));
        }
        /* 门灯特有项 */
        if(p2p_get_device_type() == DEVICE_TYPE_DOOR_LAMP ||
                p2p_get_device_type() == DEVICE_TYPE_GARDEN_LAMP||
               p2p_get_device_type() == DEVICE_TYPE_FISHEYE_LAMP)
        {
            char rgb[32];

            sprintf(rgb, "%d,%d,%d", get_para.lamp.color_red,
                    get_para.lamp.color_green,
                    get_para.lamp.color_blue);
            json_build_add_string(json, SYNC_RGB, rgb);
            json_build_add_string(json, SYNC_LIGHT_SWITCH, p2p_utoa(get_para.lamp.light_switch));
            json_build_add_string(json, SYNC_WHITE_SWITCH, p2p_utoa(get_para.lamp.white_switch));
            json_build_add_string(json, SYNC_BREATH_SWITCH, p2p_utoa(get_para.lamp.breath_switch));
            json_build_add_string(json, SYNC_LIGHT_SCHEDULE, p2p_utoa(get_para.light_schedule));
            char sunrise[32];
            char sunset[32];

            sprintf(sunrise, "%d:%d", get_para.lamp.sunrise_hour, get_para.lamp.sunrise_min);
            sprintf(sunset, "%d:%d", get_para.lamp.sunset_hour, get_para.lamp.sunset_min);
            json_build_add_string(json, SYNC_SUNRISE, sunrise);
            json_build_add_string(json, SYNC_SUNSET, sunset);
        }
        json_build_add_string(json, SYNC_BACKLIGHT, p2p_utoa(get_para.device_backlight));
        json_build_add_string(json, SYNC_LIGHT_BRIGHTNESS, p2p_utoa(get_para.light_brightness));
        json_build_add_string(json, SYNC_VOLUME, p2p_utoa(get_para.device_volume));

        syncinfo = json_build_end(json);
    }
#endif
    WebTask task;

    std::string server = get_devmng_server();
    std::string url = server+std::string(WEB_SYNC_URI);
	task.SetUrl(url.c_str());

    if(baseinfo)
    {
        task.AddPostString("baseinfo", baseinfo);
        free(baseinfo);
    }
    if(syncinfo)
    {
        task.AddPostString("syncinfo", syncinfo);
        free(syncinfo);
    }

    task.DoGetString();

    /* sync web server */
    if (0 != task.WaitTaskDone())
    {
        plog("web sync paramter failed\n"); 
        mark_invalid_server(server.c_str());
        return -1;
    }
    if(!json_is_json(task.GetResultString()))
        mark_invalid_server(server.c_str());

    const char* resp = task.GetResultString();
    plog("recv:%s\n", resp);
    if(0 != web_check_result(resp))
    {
        return -1;
    }

    char  tmp_synckey[32]= {0};

    /* parser synckey*/
    json_object_parser((char*)resp, "synckey%32s", tmp_synckey);
    if(strlen(tmp_synckey) == 0 || atoi(tmp_synckey) == 0)
    {
        plog("synckey is NULL\r\n");
        return -1;	
    }	
    else
    {
        JsonParser json((char*)resp);
        const char* data = json.GetStringConst("data");
        if(data && strlen(data))
        {			
            p2p_parser_sync_data(tmp_synckey, data);
        }	
    }
    return 0;
}

int web_report_value(p2p_report_value_t * v, int num)
{
    WebTask task;

    char *baseinfo = json_build_objects("tokenid%sphysical_id%sdevice_type%d",
            web_get_tokenid(), p2p_get_deviceid(), p2p_get_device_type());

    if(!baseinfo) return -1;
    JSON_OBJECT json = json_build_new();

    for(int i=0; i< num; i++)
    {
        JSON_OBJECT array_json = json_build_new();
        json_build_add_string(array_json, "value", v[i].value);
        json_build_add_string(array_json, "channel_id", p2p_itoa(v[i].channel));
        json_build_add_string(array_json, "type", p2p_itoa(v[i].type));
        json_build_add_object(json, p2p_itoa(i) ,array_json);
    }
    char* report_info = json_build_end(json);

    std::string server = get_devmng_server();
    std::string url = server+std::string(WEB_TIMELINE_RPT_URI);
	task.SetUrl(url.c_str());
    task.AddPostString("baseinfo", baseinfo);
    task.AddPostString("report_info", report_info);
    task.DoGetString();

    free(baseinfo);
    free(report_info);

    if(0 == task.WaitTaskDone())
    {
        plog("recv %s\n", task.GetResultString());
        if(!json_is_json(task.GetResultString()))
            mark_invalid_server(server.c_str());
        if(web_check_result(task.GetResultString()) ==0)
            return 0;
    }
    mark_invalid_server(server.c_str());
    plog(" error!!!");
    return -1;
}
int web_report_battery(p2p_report_battery_t * v, int num)
{
    char *baseinfo = json_build_objects("tokenid%sphysical_id%sdevice_type%d",
            web_get_tokenid(), p2p_get_deviceid(), p2p_get_device_type());

    if(!baseinfo) return -1;
    JSON_OBJECT json = json_build_new_array();

    for(int i=0; i< num; i++)
    {
        JSON_OBJECT array_json = json_build_new();
        json_build_add_string(array_json, "physical_id", v[i].physical_id);
        json_build_add_int(array_json, "device_type", v[i].device_type);
        //json_build_add_int(array_json, "battery_level", v[i].battery_level);
        json_build_add_string(array_json, "battery_voltage", p2p_ftoa(v[i].battery_voltage));

        json_build_array_add(json, array_json);
    }
    char* batteryinfo = json_build_end(json);

    WebTask task;
    std::string server = get_devmng_server();
    std::string url = server+std::string(WEB_TIMELINE_RPT_URI);
	task.SetUrl(url.c_str());
    task.AddPostString("baseinfo", baseinfo);
    task.AddPostString("batteryinfo", batteryinfo);
    task.DoGetString();

    free(baseinfo);
    free(batteryinfo);

    if(0 == task.WaitTaskDone())
    {
        plog("recv %s\n", task.GetResultString());
        if(!json_is_json(task.GetResultString()))
            mark_invalid_server(server.c_str());
        if(web_check_result(task.GetResultString()) ==0)
            return 0;
    }
    plog(" error!!!");
    mark_invalid_server(server.c_str());
    return -1;
}
int web_sync_value(const char* name, const char* value)
{
    char *baseinfo = NULL;	
    char *syncinfo = NULL;	
    web_sync_param_t get_para;

    plogfn();

    p2p_get_sync_paramter(&get_para);

    /* build baseinfo */
    JSON_OBJECT base_json = json_build_new();

    if(base_json)
    {
        json_build_add_string(base_json, "tokenid", web_get_tokenid());
        json_build_add_string(base_json, "physical_id", p2p_get_deviceid());
        json_build_add_string(base_json, "device_type", p2p_itoa(p2p_get_device_type()));
        json_build_add_string(base_json, "synckey", get_para.sync_key);
        baseinfo = json_build_end(base_json);
    }

    JSON_OBJECT sync_json = json_build_new();

    if(sync_json)
    {
        json_build_add_string(sync_json, name, value);
        syncinfo = json_build_end(sync_json);
    }

    WebTask task;

    std::string server = get_devmng_server();
    std::string url = server+std::string(WEB_SYNC_URI);
	task.SetUrl(url.c_str());

    if(baseinfo)
    {
        task.AddPostString("baseinfo", baseinfo);
        free(baseinfo);
    }
    if(syncinfo)
    {
        task.AddPostString("syncinfo", syncinfo);
        free(syncinfo);
    }
    task.DoGetString();

    /* sync web server */
    if (0 != task.WaitTaskDone())
    {
        plog("web sync paramter failed\n"); 
        mark_invalid_server(server.c_str());
        return -1;
    }
    if(!json_is_json(task.GetResultString()))
        mark_invalid_server(server.c_str());
    const char* resp = task.GetResultString();
    plog("recv:%s\n", resp);
    if(0 != web_check_result(resp))
    {
        return -1;
    }

    char  tmp_synckey[32]= {0};

    /* parser synckey*/
    json_object_parser((char*)resp, "synckey%32s", tmp_synckey);
    if(strlen(tmp_synckey) == 0 || atoi(tmp_synckey) == 0)
    {
        plog("synckey is NULL\r\n");
        return -1;	
    }	

    /* 只保存synckey，不同步其他参数，避免数据混乱 */	
    JsonParser json((char*)resp);
    const char* data = json.GetStringConst("data");
    if(data && strlen(data))
    {			
        p2p_parser_sync_data(tmp_synckey, data);
    }	
    return 0;
}
int web_sync_values(const p2p_sync_value_t* values, int vc)
{
    char *baseinfo = NULL;	
    char *syncinfo = NULL;	
    web_sync_param_t get_para;

    plogfn();

    p2p_get_sync_paramter(&get_para);

    /* build baseinfo */
    JSON_OBJECT base_json = json_build_new();

    if(base_json)
    {
        json_build_add_string(base_json, "tokenid", web_get_tokenid());
        json_build_add_string(base_json, "physical_id", p2p_get_deviceid());
        json_build_add_string(base_json, "device_type", p2p_itoa(p2p_get_device_type()));
        json_build_add_string(base_json, "synckey", get_para.sync_key);
        baseinfo = json_build_end(base_json);
    }

    JSON_OBJECT sync_json = json_build_new();

    if(sync_json)
    {
        for (int i = 0; i < vc; i++) 
        {
            json_build_add_string(sync_json, values[i].name, values[i].value);
        }
        syncinfo = json_build_end(sync_json);
    }

    WebTask task;

    std::string server = get_devmng_server();
    std::string url = server+std::string(WEB_SYNC_URI);
	task.SetUrl(url.c_str());

    if(baseinfo)
    {
        task.AddPostString("baseinfo", baseinfo);
        free(baseinfo);
    }
    if(syncinfo)
    {
        task.AddPostString("syncinfo", syncinfo);
        free(syncinfo);
    }
    task.DoGetString();

    /* sync web server */
    if (0 != task.WaitTaskDone())
    {
        plog("web sync paramter failed\n"); 
        mark_invalid_server(server.c_str());
        return -1;
    }
    if(!json_is_json(task.GetResultString()))
        mark_invalid_server(server.c_str());
    const char* resp = task.GetResultString();
    plog("recv:%s\n", resp);
    if(0 != web_check_result(resp))
    {
        return -1;
    }

    char  tmp_synckey[32]= {0};

    /* parser synckey*/
    json_object_parser((char*)resp, "synckey%32s", tmp_synckey);
    if(strlen(tmp_synckey) == 0 || atoi(tmp_synckey) == 0)
    {
        plog("synckey is NULL\r\n");
        return -1;	
    }	

    /* 只保存synckey，不同步其他参数，避免数据混乱 */	
    JsonParser json((char*)resp);
    const char* data = json.GetStringConst("data");
    if(data && strlen(data))
    {			
        p2p_parser_sync_data(tmp_synckey, data);
    }	
    return 0;
}

int web_upload_sub_device_list(int num, const sub_device_info *sd_list)
{
    char *baseinfo = NULL;	
    char *relation = NULL;	
    web_sync_param_t get_para;

    plogfn();

    p2p_get_sync_paramter(&get_para);

    /* build baseinfo */
    JSON_OBJECT base_json = json_build_new();

    if(base_json)
    {
        json_build_add_string(base_json, "tokenid", web_get_tokenid());
        json_build_add_string(base_json, "physical_id", p2p_get_deviceid());
        json_build_add_string(base_json, "device_type", p2p_itoa(p2p_get_device_type()));
        json_build_add_string(base_json, "synckey", get_para.sync_key);
        baseinfo = json_build_end(base_json);
    }

    void* json_array = json_build_new_array();

    for(int i=0; i<num; i++)
    {
        JSON_OBJECT sd_json = json_build_new();
        if(sd_json)
        {
            json_build_add_string(sd_json, "physical_id", sd_list[i].physical_id);
            json_build_add_string(sd_json, "device_name", sd_list[i].device_name);
            json_build_add_int(sd_json, "device_type", sd_list[i].device_type);
            json_build_add_int(sd_json, "channel_id", sd_list[i].channel_id);
            json_build_add_string(sd_json, "local_pwd", sd_list[i].local_pwd);
            json_build_add_int(sd_json, "use_on", sd_list[i].use_on);
            json_build_array_add(json_array, sd_json);
        }
    }

    JSON_OBJECT relation_json = json_build_new();

    if(relation_json)
    {
        json_build_add_object(relation_json, "list", json_array);
        relation = json_build_end(relation_json);
        plogfn();
    }

    WebTask task;

    std::string server = get_devmng_server();
    std::string url = server+std::string(WEB_SYNC_URI);
	task.SetUrl(url.c_str());

    if(baseinfo)
    {
        task.AddPostString("baseinfo", baseinfo);
        free(baseinfo);
    }
    if(relation)
    {
        task.AddPostString("relation", relation);
        free(relation);
    }
    task.DoGetString();

    /* sync web server */
    if (0 != task.WaitTaskDone())
    {
        plog("web sync paramter failed\n"); 
        mark_invalid_server(server.c_str());
        return -1;
    }
    if(!json_is_json(task.GetResultString()))
        mark_invalid_server(server.c_str());
    const char* resp = task.GetResultString();
    plog("recv:%s\n", resp);
    if(0 != web_check_result(resp))
    {
        return -1;
    }

    char  tmp_synckey[32]= {0};

    /* parser synckey*/
    json_object_parser((char*)resp, "synckey%32s", tmp_synckey);
    if(strlen(tmp_synckey) == 0 || atoi(tmp_synckey) == 0)
    {
        plog("synckey is NULL\r\n");
        return -1;	
    }	

    /* 只保存synckey，不同步其他参数，避免数据混乱 */	

    JsonParser json((char*)resp);
    const char* data = json.GetStringConst("data");
    if(data && strlen(data))
    {			
        p2p_parser_sync_data(tmp_synckey, data);
    }	

    return 0;
}

static int web_upload_pt_preset_imp(int flag, const char* trigger_id,
        const char* preset_name, int  preset_on,
        const char* original_name, char* image_url)
{
    WebTask task;

    std::string server = get_devmng_server();
    std::string url = server+std::string(WEB_PRESET_SET_URI);
	task.SetUrl(url.c_str());


    task.AddPostString("tokenid", web_get_tokenid());
    task.AddPostString("physical_id", p2p_get_deviceid());
    task.AddPostString("dev_type", p2p_itoa(p2p_get_device_type()));
    task.AddPostString("flag", p2p_itoa(flag));
    if(trigger_id)
        task.AddPostString("trigger_id", trigger_id);
    task.AddPostString("image_url", image_url);
    if(flag == 1)
    {
        P2P_ASSERT(original_name != NULL, -1);

        // 修改preset_name 时， 必须带上原preset_name
        // name 为新的 preset_name
        // preset_name 是旧的

        if(preset_name && strcmp(original_name, preset_name))
            task.AddPostString("name", preset_name);
        task.AddPostString("preset_name", original_name);
    }
    else
    {
        task.AddPostString("preset_name", preset_name);
        /*只在增加时上传此字段，避免数据不同步*/
        task.AddPostString("preset_on", p2p_itoa(preset_on));		
    }

    task.DoGetString();

    if (0 != task.WaitTaskDone())
    {
        plog("web upload preset failed\n"); 
        mark_invalid_server(server.c_str());
        return -1;
    }
    if(!json_is_json(task.GetResultString()))
        mark_invalid_server(server.c_str());

    const char* resp = task.GetResultString();
    plog("recv:%s\n", resp);
    if(0 != web_check_result(resp))
    {
        return -1;
    }
    return 0;
}

int web_upload_pt_preset(int flag, const char* trigger_id,
        const char* preset_name, int  preset_on,
        const char* image_name, const char* original_name,
        char** img_url)
{
    plogfn();

    int try_count = 3;
    char image_url[128]="";

    while(try_count-- > 0)
    {
        if(0 == web_upload_preset_cover(trigger_id, image_name, image_url))
            break;
    }

    if(strlen(image_url)==0)
    {
        plog("upload preset cover failed!\n");
        return -1;
    }

    try_count = 3;
    while(try_count-- >0)
    {
        if(0 == web_upload_pt_preset_imp(flag, trigger_id, preset_name, preset_on, original_name, image_url))
            break;
    }

    if(try_count > 0)
    {
        *img_url  = strdup(image_url);
        return 0;	
    }
    else
    {
        return -1;
    }
}

int web_get_voicemesage(const char* file_id, const char* save_to)
{
    plogfn();
    int try_count = 10;

    int reserved_kbytes = 256;

    const char* reserved_str = getenv("P2P_RESERVED_KBYTES");
    if(reserved_str)
    {
        reserved_kbytes = atoi(reserved_str);
    }
    while(try_count-- >0)
    {
        WebTask task;

        std::string server = get_fileserver();
        std::string url = server+std::string(WEB_GETFILE_URI);
        task.SetUrl(url.c_str());
        task.AddPostString("tokenid", web_get_tokenid());
        task.AddPostString("physical_id", p2p_get_deviceid());
        task.AddPostString("url", file_id);

        task.DoGetFile();
        if (0 != task.WaitTaskDone())
        {
            plog("download voice message failed!\n"); 
            mark_invalid_server(server.c_str());
            sleep(3);
            continue;
        }
        int fsize = get_filesize(task.GetFilePath());
        fsize = ((float)((float)fsize/1024)/160)*164 ;
        int idle_size = p2p_get_idle_kbytes(save_to);
        plog("file size [%u]KB, idle size [%u]KB, reserved_size [%u]\n",
                fsize, idle_size, reserved_kbytes );
        if(fsize + reserved_kbytes >  idle_size)
        {
            plog("not enough space for file size [%u]KB, idle size [%u]KB, reserved_size [%u]\n",
                    fsize, idle_size, reserved_kbytes );
            unlink(task.GetFilePath());
            return 0;
        }
        p2p_add_hi_head_to_g711(task.GetFilePath(), save_to);
        unlink(task.GetFilePath());
        return 0;
    }
    return -1;
}

int web_report_adding_devices(adding_device_info* ad, int num)
{
    plogfn();

    WebTask task;

    std::string server = get_devmng_server();
    std::string url = server+std::string(WEB_ADDING_DEVICE_URI);
	task.SetUrl(url.c_str());

    task.AddPostString("tokenid", web_get_tokenid());
    task.AddPostString("physical_id", p2p_get_deviceid());

    void* json_array = json_build_new_array();

    for(int i=0; i<num; i++)
    {
        JSON_OBJECT sd_json = json_build_new();
        if(sd_json)
        {
            json_build_add_string(sd_json, "physical_id", ad[i].physical_id);
            json_build_add_int(sd_json, "dev_type", ad[i].dev_type);
            json_build_array_add(json_array, sd_json);
        }
    }
    char* json = json_build_end(json_array);
    if(json)
    {
        task.AddPostString("list", json);
        free(json);
    }
    task.DoGetString();

    /* sync web server */
    if (0 != task.WaitTaskDone())
    {
        plog("report adding devices failed!\n"); 
        mark_invalid_server(server.c_str());
        return -1;
    }
    if(!json_is_json(task.GetResultString()))
        mark_invalid_server(server.c_str());
    const char* resp = task.GetResultString();
    plog("recv:%s\n", resp);
    if(0 != web_check_result(resp))
    {
        return -1;
    }
    return 0;
}

int web_set_upgrade_state(int type, const char* info)
{
    WebTask task;

    task.SetUrl(WEB_SET_UPGRADE_STATE);

    task.AddPostString("tokenid", web_get_tokenid());
    task.AddPostString("physical_id", p2p_get_deviceid());
    task.AddPostString("st", p2p_itoa(type));

    if(!info)
    {
        plog("info == NULL!!!\n");
        return -1;
    }
    if(type == 0 || type == 1)
    {
        task.AddPostString("data", info);
    }
    else if(type == 2)
    {
        char error[1024];
        snprintf(error, sizeof(error), "{\"result\":\"2\", \"message\":\"%s\", \"data\":\"none\"}", info);
        error[sizeof(error)-1]='\0';
        task.AddPostString("data", error);
    }
    else
    {
        plog("unknow type %d\n", type);
        return -1;
    }
    task.DoGetString();

    /* sync web server */
    if (0 != task.WaitTaskDone())
    {
        plog("failed!\n"); 
        return -1;
    }
    const char* resp = task.GetResultString();
    plog("recv:%s\n", resp);
    if(0 != web_check_result(resp))
    {
        return -1;
    }
    return 0;
}

int web_rest_rpt(sleep_server_info *info)
{
	plogfn();

    WebTask task;

    std::string server = get_devmng_server();
    std::string url = server+std::string(WEB_REST_RPT_URI);
	task.SetUrl(url.c_str());

	task.AddPostString("tokenid", web_get_tokenid());
	task.AddPostString("device_type", p2p_itoa(p2p_get_device_type()));
	task.DoGetString();

	if (0 != task.WaitTaskDone())
	{
		plog("error!!!");
        mark_invalid_server(server.c_str());
		return -1;
	}
    if(!json_is_json(task.GetResultString()))
        mark_invalid_server(server.c_str());
	plog("recv %s\n", task.GetResultString());
	if (0 != web_check_result(task.GetResultString()))
	{
		plog("result not ok\n");
		return -1;
	}

	const char* resp = task.GetResultString();
	char data[1024] = {0};
	char rest_address[128] = {0};
	char *p;

    json_object_parser((char*)resp, "data%1024s", data);
	plog("data %s\n", data);
	json_object_parser(data, "auth_code%64s", info->auth_code);
	plog("auth_code %s\n", info->auth_code);
	json_object_parser(data, "rest_address%128s", rest_address);
	plog("rest_address %s\n", rest_address);
	p = strchr(rest_address, ':');
	*p = '\0';
	strcpy(info->server_ip, rest_address);
	info->server_port = atoi(p+1);

	return 0;
}

int web_get_timezonelist(p2p_timezonelist_t *list, int *num)
{
    WebTask task;

    std::string server = get_devmng_server();
    std::string url = server+std::string(WEB_GET_TIMEZONELIST);
	task.SetUrl(url.c_str());

    task.AddPostString("tokenid", web_get_tokenid());
    task.DoGetString();

    if (0 != task.WaitTaskDone())
    {
        plog("error !!!\n"); 
        mark_invalid_server(server.c_str());
        return -1;
    }
    if(!json_is_json(task.GetResultString()))
        mark_invalid_server(server.c_str());
    const char* resp = task.GetResultString();
    plog_str(resp);
    if(0 != web_check_result(resp))
    {
        return -1;
    }

    JsonParser json((char*)resp);
    const char* data = json.GetStringConst("data");
    if(data && strlen(data))
    {			
        JSON_OBJECT jp = json_parser_new(data);
        int len = json_parser_get_array_size(jp);

        if(len > *num) 
        {
            json_parser_free(jp);
            *num = len;
            return -2;
        }

        *num = len;
        for(int i = 0; i<len; i++)
        {
            JSON_OBJECT tz = json_parser_array_get_object_by_idx(jp, i);
            strncpy(list[i].timezone, json_parser_get_string_const(tz, "timezone"), sizeof(list[i].timezone)-1);
            strncpy(list[i].desc_zh, json_parser_get_string_const(tz, "desc_zh"), sizeof(list[i].desc_zh)-1);
            strncpy(list[i].desc_en, json_parser_get_string_const(tz, "desc_en"), sizeof(list[i].desc_en)-1);
            strncpy(list[i].offset, json_parser_get_string_const(tz, "offset"), sizeof(list[i].offset)-1);
            json_parser_free(tz);
        }
        json_parser_free(jp);
    }	
    return 0;
}
int web_report_thermostat_stat(p2p_thermostat_stat_t * st)
{
    WebTask task;

    std::string server = get_devmng_server();
    std::string url = server+std::string(WEB_TIMELINE_RPT_URI);
	task.SetUrl(url.c_str());

    char *baseinfo = json_build_objects("tokenid%sphysical_id%sdevice_type%d",
            web_get_tokenid(), p2p_get_deviceid(), p2p_get_device_type());

    if(!baseinfo) return -1;

    JSON_OBJECT json = json_build_new();

    json_build_add_string(json, "physical_id", st->physical_id);
    json_build_add_int(json, "temperature", st->temperature);
    json_build_add_int(json, "vent_switch", st->vent_switch);
    json_build_add_int(json, "current_temp", st->current_temp);
    if(st->vent_status == 0 || st->vent_status == 1)
        json_build_add_int(json, "vent_status", st->vent_status);

    char* thermostatinfo = json_build_end(json);

    task.SetUrl(url.c_str());
    task.AddPostString("baseinfo", baseinfo);
    task.AddPostString("thermostatinfo", thermostatinfo);
    task.DoGetString();

    free(baseinfo);
    free(thermostatinfo);

    if(0 == task.WaitTaskDone())
    {
        plog("recv %s\n", task.GetResultString());
        if(!json_is_json(task.GetResultString()))
            mark_invalid_server(server.c_str());
        if(web_check_result(task.GetResultString()) ==0)
            return 0;
    }
    mark_invalid_server(server.c_str());
    plog(" error!!!");
    return -1;
}
int web_report_waterholeinfo(const char* physical_id, int hole_id, int watering)
{
    WebTask task;

    std::string server = get_devmng_server();
    std::string url = server+std::string(WEB_TIMELINE_RPT_URI);
	task.SetUrl(url.c_str());

    char *baseinfo = json_build_objects("tokenid%sphysical_id%sdevice_type%d",
            web_get_tokenid(), p2p_get_deviceid(), p2p_get_device_type());

    if(!baseinfo) return -1;
    JSON_OBJECT json = json_build_new_array();

    for(int i=0; i< 1; i++)
    {
        JSON_OBJECT array_json = json_build_new();
        json_build_add_string(array_json, "physical_id", physical_id);
        json_build_add_int(array_json, "watering", watering);
        json_build_add_int(array_json, "hole_id", hole_id);

        json_build_array_add(json, array_json);
    }
    char* waterholeinfo = json_build_end(json);

    task.SetUrl(url.c_str());
    task.AddPostString("baseinfo", baseinfo);
    task.AddPostString("waterholeinfo", waterholeinfo);
    task.DoGetString();

    free(baseinfo);
    free(waterholeinfo);

    if(0 == task.WaitTaskDone())
    {
        plog("recv %s\n", task.GetResultString());
        if(!json_is_json(task.GetResultString()))
            mark_invalid_server(server.c_str());
        if(web_check_result(task.GetResultString()) ==0)
            return 0;
    }
    mark_invalid_server(server.c_str());
    plog(" error!!!");
    return -1;
}

int web_report_lockinfo(const char* physical_id, int locked)
{
    WebTask task;

    std::string server = get_devmng_server();
    std::string url = server+std::string(WEB_TIMELINE_RPT_URI);
	task.SetUrl(url.c_str());

    char *baseinfo = json_build_objects("tokenid%sphysical_id%sdevice_type%d",
            web_get_tokenid(), p2p_get_deviceid(), p2p_get_device_type());

    if(!baseinfo) return -1;
    JSON_OBJECT json = json_build_new_array();

    for(int i=0; i< 1; i++)
    {
        JSON_OBJECT array_json = json_build_new();
        json_build_add_string(array_json, "physical_id", physical_id);
        json_build_add_int(array_json, "locked", locked);

        json_build_array_add(json, array_json);
    }
    char* lockinfo = json_build_end(json);

    task.SetUrl(url.c_str());
    task.AddPostString("baseinfo", baseinfo);
    task.AddPostString("lockinfo", lockinfo);
    task.DoGetString();

    free(baseinfo);
    free(lockinfo);

    if(0 == task.WaitTaskDone())
    {
        plog("recv %s\n", task.GetResultString());
        if(!json_is_json(task.GetResultString()))
            mark_invalid_server(server.c_str());
        if(web_check_result(task.GetResultString()) ==0)
            return 0;
    }
    mark_invalid_server(server.c_str());
    plog(" error!!!");
    return -1;
}
//return 0 success, return 1 failed but no need to try , -1 please try again
int web_upload_alert(const std::map<std::string, std::string> &alert_infos, char* alarm_id)
{	
    if(p2p_base()->token_valid == 0)
    {
        web_login(p2p_base(), 10);
        if(p2p_base()->token_valid == 0)
        {
            //可能是登陆服务器出问题了，等待会再重试
            sleep(p2p_get_rand(1,5));
            return 1;
        }
    }
	WebTask task;

    std::string server = get_alerts_server();
    std::string url = server + std::string(WEB_ALARM_URI);
	task.SetUrl(url.c_str());

	task.AddPostString("tokenid", web_get_tokenid());
	task.AddPostString("physical_id", p2p_get_deviceid());

    std::map<std::string, std::string>::const_iterator it;
    for(it=alert_infos.begin(); it!=alert_infos.end(); it++)
    {
        task.AddPostString(it->first.c_str(), it->second.c_str());
    }
    
	task.DoGetString();
	
	if (0 != task.WaitTaskDone())
	{
		plog("error!!!");
        mark_invalid_server(server.c_str());
		return -1;
	}
	plog("recv %s\n", task.GetResultString());

	JsonParser json(task.GetResultString());

	if (0 != web_check_result(task.GetResultString()))
	{
		plog("result not ok\n");
        if(!json_is_json(task.GetResultString()))
        {
            mark_invalid_server(server.c_str());
            return -1;
        }
	}	
    int ret = 1;
    if(strcmp(json.GetStringConst("result"), "ok") == 0||strcmp(json.GetStringConst("result"), "0")==0)
    {
        ret = 0;
    }

	const char* id = json.GetStringConst("id");

	if(id) strcpy(alarm_id, id);
	
	return ret;
}

int web_upload_alert_picture(const char* alarm_id, const std::vector<std::string>& pic_paths)
{
	WebTask task;

    std::string server = get_fileserver();
    std::string url = server+std::string(FILESERVER_UPLOAD_URI);
	task.SetUrl(url.c_str());
	task.AddPostString("alarm_id", alarm_id);
	task.AddPostString("tokenid", web_get_tokenid());
	task.AddPostString("physical_id", p2p_get_deviceid());
    for(int i=0; i<pic_paths.size(); i++)
        task.AddPostPicture("image_name[]", pic_paths[i].c_str(), NULL);

	task.DoGetString();
	
	if(0 != task.WaitTaskDone())
	{
		plog("error!\n");
        mark_invalid_server(server.c_str());
		return -1;
	}
	plog("recv %s\n", task.GetResultString());

	if (0 != web_check_result(task.GetResultString()))
	{
		plog("result not ok\n");
        if(!json_is_json(task.GetResultString()))
            mark_invalid_server(server.c_str());
		return -1;
	}		
	return 0;
}

int web_upload_alert_video(const char* alarm_id, const char* file_path, const char* file_name, int video_last, char* video_file_url, int* video_file_size)
{
    WebTask task;

    std::string server = get_fileserver();
    std::string url = server+std::string(FILESERVER_UPLOAD_URI);
	task.SetUrl(url.c_str());
    task.AddPostString("alarm_id", alarm_id);
    //task.AddPostString("file_type", p2p_itoa(1));
    task.AddPostString("tokenid", web_get_tokenid());
    task.AddPostString("physical_id", p2p_get_deviceid());
    task.AddPostString("video_last", p2p_itoa(video_last));
    task.AddPostFileChunked("video_file_name", file_path, file_name);

    task.DoGetString();
    
    if(0 != task.WaitTaskDone())
	{
		plog("error!\n");
        mark_invalid_server(server.c_str());
		return -1;
	}
	plog("recv %s\n", task.GetResultString());

	if (0 != web_check_result(task.GetResultString()))
	{
		plog("result not ok\n");
        if(!json_is_json(task.GetResultString()))
            mark_invalid_server(server.c_str());
		return -1;
	}		
    if(video_file_url && video_file_size)
    {
        JsonParser jp(task.GetResultString());

        if(jp.GetStringConst("postdata"))
        {
            JsonParser jf(jp.GetStringConst("postdata"));

            if(jf.GetStringConst("video_file_url"))
            {
                strcpy(video_file_url, jf.GetStringConst("video_file_url"));
            }
            if(jf.GetStringConst("video_file_size"))
            {
                *video_file_size = jf.GetIntValue("video_file_size");
            }
        }
    }
    return 0;
}

int web_upload_cover_picture(const char* file_path, int channel)
{
	WebTask task;

    std::string server = get_fileserver();
    std::string url = server+std::string(WEB_REPORT_PICTURE_URI);
	task.SetUrl(url.c_str());
	task.AddPostString("tokenid", web_get_tokenid());
	task.AddPostString("physical_id", p2p_get_deviceid());
	task.AddPostString("channel", p2p_itoa(channel));
	task.AddPostPicture("image_name", file_path);

	task.DoGetString();
	
	if(0 != task.WaitTaskDone())
	{
		plog("error!\n");
        mark_invalid_server(server.c_str());
		return -1;
	}
	plog("recv %s\n", task.GetResultString());

	if (0 != web_check_result(task.GetResultString()))
	{
		plog("result not ok\n");
        if(!json_is_json(task.GetResultString()))
            mark_invalid_server(server.c_str());
		return -1;
	}		
	return 0;
}
int web_upload_preset_cover(const char* trigger_id, const char* pic, char* image_url)
{
	WebTask task;

    std::string server = get_fileserver();
    std::string url = server+std::string(WEB_PRESET_COVER_URI);
	task.SetUrl(url.c_str());
	task.AddPostString("tokenid", web_get_tokenid());
	task.AddPostString("physical_id", p2p_get_deviceid());
	task.AddPostString("trigger_id", trigger_id);
	task.AddPostPicture("image_name", pic);

	task.DoGetString();
	
	if(0 != task.WaitTaskDone())
	{
		plog("error!\n");
        mark_invalid_server(server.c_str());
		return -1;
	}
	plog("recv %s\n", task.GetResultString());

	if (0 != web_check_result(task.GetResultString()))
	{
		plog("result not ok\n");
        if(!json_is_json(task.GetResultString()))
            mark_invalid_server(server.c_str());
		return -1;
	}		
    JsonParser jp(task.GetResultString());

    const char* img = jp.GetStringConst("image_url");
    if(img)
    {
        strcpy(image_url, img);
    }
	return 0;

}

int web_upload_file(const char* fpath, int file_type, char* video_file_url)
{
    WebTask task;

    std::string server = get_fileserver();
    std::string url = server+std::string(WEB_UPLOADFILE_URI);
	task.SetUrl(url.c_str());
    task.AddPostString("storage_flag", p2p_itoa(file_type));
    task.AddPostString("tokenid", web_get_tokenid());
    task.AddPostString("physical_id", p2p_get_deviceid());
    char fname[256]="";
    sprintf(fname, "%d_%s_%s", file_type, web_get_tokenid(), get_src_name(fpath) );
    task.AddPostFileChunked("file_name", fpath, fname);

    task.DoGetString();
    
    if(0 != task.WaitTaskDone())
	{
		plog("error!\n");
        mark_invalid_server(server.c_str());
		return -1;
	}
	plog("recv %s\n", task.GetResultString());

	if (0 != web_check_result(task.GetResultString()))
	{
		plog("result not ok\n");
        if(!json_is_json(task.GetResultString()))
            mark_invalid_server(server.c_str());
		return -1;
	}		
    if(video_file_url)
    {
        JsonParser jp(task.GetResultString());

        if(jp.GetStringConst("file_url"))
        {
            strcpy(video_file_url, jp.GetStringConst("file_url"));
        }
    }
    return 0;
}
int web_update_alert(const char* alert_ids, const char* video_file_url, int video_file_size, int video_last)
{
	WebTask task;

    std::string server = get_alerts_server();
    std::string url = server + std::string(WEB_UPDATE_ALERT_URI);
	task.SetUrl(url.c_str());

	task.AddPostString("tokenid", web_get_tokenid());
	task.AddPostString("physical_id", p2p_get_deviceid());
	task.AddPostString("id", alert_ids);
	task.AddPostString("video_file_url", video_file_url);
	task.AddPostString("video_file_size", p2p_itoa(video_file_size));
	task.AddPostString("video_last", p2p_itoa(video_last));

	task.DoGetString();
	
	if (0 != task.WaitTaskDone())
	{
		plog("error!!!");
        mark_invalid_server(server.c_str());
		return -1;
	}
	plog("recv %s\n", task.GetResultString());

	JsonParser json(task.GetResultString());

	if (0 != web_check_result(task.GetResultString()))
	{
		plog("result not ok\n");
        if(!json_is_json(task.GetResultString()))
        {
            mark_invalid_server(server.c_str());
            return -1;
        }
	}	
    int ret = 1;
    if(strcmp(json.GetStringConst("result"), "ok") == 0||strcmp(json.GetStringConst("result"), "0")==0)
    {
        ret = 0;
    }
	else if(strcmp(json.GetStringConst("result"), "1002") == 0)
	{
		//token 失效之后需要重传告警
        ret = -1;
	}

	return ret;
}

int web_lpr_rpt(const char* pics[], int pic_num)
{
	WebTask task;

    std::string server = get_fileserver();
    std::string url = server + std::string(WEB_LPR_RPT);
	task.SetUrl(url.c_str());

    task.AddPostString("tokenid", web_get_tokenid());
    for(int i=0; i< pic_num; i++)
    {
        task.AddPostPicture("image_name[]", pics[i], get_src_name(pics[i]));
    }
    task.DoGetString();

    if(0 != task.WaitTaskDone())
	{
		plog("error!\n");
        mark_invalid_server(server.c_str());
		return -1;
	}
	plog("recv %s\n", task.GetResultString());

	if (0 != web_check_result(task.GetResultString()))
	{
		plog("result not ok\n");
        if(!json_is_json(task.GetResultString()))
            mark_invalid_server(server.c_str());
		return -1;
	}		
    return 0;
}

int web_face_rpt(const char* pic_path, int face_num, p2p_face_pos_t *pos)
{
	WebTask task;

    std::string server = get_fileserver();
    std::string url = server + std::string(WEB_FACE_RPT);
	task.SetUrl(url.c_str());

    task.AddPostString("tokenid", web_get_tokenid());
    task.AddPostString("physical_id", p2p_get_deviceid());
    task.AddPostPicture("image_name", pic_path, get_src_name(pic_path));

    JSON_OBJECT json = json_build_new_array();

    for(int i=0; i< face_num; i++)
    {
        JSON_OBJECT array_json = json_build_new();
        json_build_add_int(array_json, "x", pos[i].x);
        json_build_add_int(array_json, "y", pos[i].y);
        json_build_add_int(array_json, "w", pos[i].w);
        json_build_add_int(array_json, "h", pos[i].h);
        json_build_array_add(json, array_json);
    }
    char* rects = json_build_end(json);
    task.AddPostString("rects", rects);

    free(rects);

    task.DoGetString();

    if(0 != task.WaitTaskDone())
	{
		plog("error!\n");
        mark_invalid_server(server.c_str());
		return -1;
	}
	plog("recv %s\n", task.GetResultString());

	if (0 != web_check_result(task.GetResultString()))
	{
		plog("result not ok\n");
        if(!json_is_json(task.GetResultString()))
            mark_invalid_server(server.c_str());
		return -1;
	}		
    return 0;
}

int web_get_voice_file(const char* user_id, char* fpath)
{
    WebTask task;

    std::string server = get_fileserver();
    std::string url = server+std::string(WEB_GETFILE_URI);
    task.SetUrl(url.c_str());
    task.AddPostString("tokenid", web_get_tokenid());
    task.AddPostString("physical_id", p2p_get_deviceid());
    char get_url[128]={0};
    sprintf(get_url, "http://127.0.0.1/v1/swift/img/%s.wav", user_id);
    task.AddPostString("url", get_url);

    task.DoGetFile();
    if (0 != task.WaitTaskDone())
    {
        plog("download voice message failed!\n"); 
        mark_invalid_server(server.c_str());
        return -1;
    }
    if(fpath)
        strcpy(fpath, task.GetFilePath());
    return 0;
}
