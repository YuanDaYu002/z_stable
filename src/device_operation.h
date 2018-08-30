
#ifndef P2P_DEVICE_OPERATION_H

#define P2P_DEVICE_OPERATION_H

#include <string>
#include <stdint.h>
#include "zmd_msg.h"

#include "BufferManage.h"
#include "ModuleFuncInterface.h"
#include "systemparameterdefine.h"
#include "p2p_interface.h"

#define SYNC_CHANGEFLAG			"changeFlag" 			/* 设备本地参数修改位*/

int  p2p_set_init_info(p2p_init_info_t* info, unsigned int info_len);

bool p2p_is_p2p_device();

const char* p2p_get_deviceid();

//get io alarm channel number
int p2p_get_device_io_chlnum();
//check if the channel has io alarm
int p2p_is_channel_io_alarm(int ionum, int chlnum);
//-------------------------------------------

//update 
int  p2p_update_system(const char* url);

//get microphone
int  p2p_hold_mic(void* linkid, const char* uid);
//free microphone
int  p2p_free_mic(void* linkid, const char* uid);

void p2p_decode_audio(void* data, int len);


//for upnp
int  p2p_is_upnp_success();

int  p2p_get_upnp_video_port();

int  p2p_get_local_video_port();


int p2p_ptz_operat(int chl, int ptz_cmd, int para0, int para1);
int p2p_ptz_get_preset(char* buf, int* len);

//for transfer outside streamtype to inner usr channel type
int p2p_get_usrid_type(int request_type);

//for transfer outside streamtype to inner streamtype
int p2p_get_stream_type(int request_type);
/*************************************************
 * test if channel has video
 * streamtype: 0 qvga, 1 vga, 2 720p
 *************************************************/
int p2p_is_channel_valid(int chlnum, int streamtype);

//return media id
//@streamtype: 0 qvga, 1 vga, 2 720p
void* p2p_get_media_id(int chlnum, int streamtype);
void  p2p_free_media_id(void* id);
int   p2p_set_i_frame(void* id);
int   p2p_set_current_i_frame(void* id);
int   p2p_set_i_frame_by_curpos(void* id);
int   p2p_set_i_frame_by_second(void* id, int sec = 0);
int   p2p_get_one_frame(void* id, unsigned char** data, void *frameInfo);

int   p2p_get_device_chlnum();

//0 IPC, 1 NVR, 2 DVR
int p2p_get_device_type();
// get device's ability
uint32_t p2p_get_device_capacity();

uint32_t p2p_get_device_extend_capacity();

unsigned long long p2p_get_device_supply_capacity();

int p2p_nvr_get_valid_buftype(int chlnum, int streamtype);

//@type 0 capture for cover; 1 capture for alert video cover
const char* p2p_get_device_capture(char* pic_path, int chl=0, int type=0);

//force i frame
int p2p_force_i_frame(int chl, int streamtype);


const char* p2p_get_config_dir();

const char* p2p_get_device_versions(char *versions);

const char* p2p_get_device_name();

struct SD_CARD_STAT
{
	/*
	0￡oSD?¨ò??D?y3￡
	1￡oéè±??TSD?¨
	2￡oSD?¨?′??ê??ˉ
	3￡oSD?¨?y?ú??ê??ˉ?D
	*/
	int status;
	//in MB
	int free_size;
	//in MB
	int used_size;
};

int p2p_get_sd_card_status(SD_CARD_STAT* stat);

int p2p_send_cover_pic(int channel);

int p2p_format_sd_card();

int p2p_set_device_timezone(const char* timezon_id, int offset);

int p2p_set_timezone_offset(int offset);

int p2p_get_sync_cvr_on();

int p2p_get_sync_paramter(web_sync_param_t* param);

int p2p_set_sync_paramter(web_sync_param_t* param);

int p2p_is_restored();

void p2p_handle_para_change(web_sync_param_t* sync_data);

int p2p_parser_sync_data(char * sync_key, const char * data);

void p2p_broadcast_device();

bool p2p_wifi_connected();

int  p2p_get_playback_list_by_date(int chl, const char* date,  p2p_record_info_t **list, int *num);

extern bool p2p_is_device_online();

int p2p_get_local_net_info(const char *dev_name, char *ipv4, char *mask, char *mac);

int p2p_get_gateway_info(const char *dev_name, char *gw_ip, char *gw_mac);

const char *p2p_get_network_interface();

int p2p_network_ok();

int p2p_get_smartlink_mark();

void p2p_clear_smartlink_mark();

void p2p_get_wifi_info(char* ssid, char* pwd);

void p2p_md5_string(const char* str, char* out);

int p2p_is_device_on();

int p2p_check_device_ext_ability(extend_ability_name_e ab);

int p2p_check_device_new_ability(new_ability_name_e ab);

int  p2p_get_wifi_router_mac(char *mac);

void p2p_reset_device();

void p2p_reboot_device();

int  p2p_add_hi_head_to_g711(const char* in_file, const char* out_file);

void p2p_handle_answer_call();

void p2p_play_voice(const char* voice_message_id);

void p2p_on_refresh_cover();

void p2p_on_hang_up();

void p2p_on_refuse_answer();

void p2p_handle_timing(int timestamp);

int  p2p_get_server_timestamp_imp();

int  p2p_is_ntp_enable();

/* check if one date has any record files */
int  p2p_if_have_record_file_by_date(struct tm time, int channel);

void p2p_on_video_request(int stream_type, int channel);

void p2p_on_video_release(int stream_type, int channel);

/* if somebody watching the video */
int  p2p_get_request_video_status(int stream_type, int channel);

const char* p2p_get_ap_id(char** ap_usrid, int *number);

void p2p_on_device_online(int status);

int p2p_set_ap_mode(int mode , int device_count, const char* extra_para);

int p2p_get_cvr_interval();

int p2p_write_upgrade_log();

int p2p_is_boot_after_upgrade();

int p2p_upload_upgrade_success();

int p2p_set_buzzer(int oper);

int p2p_md_region_ctrl(int op_type, p2p_md_region_t *reg);

int p2p_set_thermostatinfo(p2p_thermostatinfo_t *info);

int p2p_watering_ctrl(p2p_watering_ctrl_t *wc);

int p2p_get_curtain_stat(int *curtain_status, int *screen_status);

int p2p_curtain_ctrl(int op_type, int is_curtain);

int p2p_preset_ctrl(int op_type, const char* physical_id, const char* preset_name, const char* new_name, char** image_url);

int p2p_lock_ctrl(int type, const char* key_info);

#endif /* end of include guard: P2P_DEVICE_OPERATION_H */
