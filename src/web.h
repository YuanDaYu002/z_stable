#ifndef WEB_H
#define WEB_H

#include <vector>
#include <string>
#include <map>
#include "p2p_interface.h"
#include "p2p_def.h"

int web_init();

//token will reflesh when login success
int web_login(p2p_base_t *base, int timeout = 20);

const char* web_get_tokenid();

//post report info
int web_report_info(const char* usrid, const char* tokenid, int timeout);

int web_report_upnp(const char *tokenid, 
	int upnp_video_port,
	int local_video_port,
	const char* dev_mac,
	const char* dev_ip,
	const char* dev_mask,
	const char* gw_mac,
	const char* gw_ip,
	const char* ssid);

int web_sync_paramter();

int web_check_result(const char* json);

int web_report_value(p2p_report_value_t * v, int num);

int web_report_battery(p2p_report_battery_t * v, int num);

int web_upload_sub_device_list(int num, const sub_device_info *sd_list);

int web_upload_pt_preset(int flag, const char* trigger_id,
	const char* preset_name, int  preset_on,
	const char* image_name, const char* original_name,
	char** img_url);

int web_get_voicemesage(const char* url, const char* save_to);

int web_report_adding_devices(adding_device_info* ad, int num);

int web_sync_value(const char* name, const char* value);

int web_sync_values(const p2p_sync_value_t* values, int vc);

/*
 * @type 0 upgrade success, @info=versions;
 *       1 download progress, @info=progress 0-100;
 *       2 upgrade error, @info=error info;
 */
int web_set_upgrade_state(int type, const char* info);

int web_rest_rpt(sleep_server_info *info);

int web_get_timezonelist(p2p_timezonelist_t *list, int *num);

int web_report_thermostat_stat(p2p_thermostat_stat_t * st);

int web_report_waterholeinfo(const char* physical_id, int hole_id, int watering);

int web_report_lockinfo(const char* physical_id, int locked);

//return 0 success, return 1 failed but no need to retry , -1 please try again
int web_upload_alert(const std::map<std::string, std::string> &alert_infos, char* alarm_id);

int web_upload_alert_picture(const char* alarm_id, const std::vector<std::string>& pic_paths);

int web_upload_alert_video(const char* alarm_id, const char* file_path, const char* file_name, int video_last, char* video_file_url, int* video_file_size);

int web_upload_cover_picture(const char* file_path, int channel);

int web_upload_preset_cover(const char* trigger_id, const char* pic, char* image_url);

// file_type 0: alerts
int web_upload_file(const char* fpath, int file_type, char* video_file_url);

int web_update_alert(const char* alert_ids, const char* video_file_url, int video_file_size, int video_last);

int web_lpr_rpt(const char* pics[], int pic_num);

int web_face_rpt(const char* pic_path, int face_num, p2p_face_pos_t *pos);

int web_get_voice_file(const char* user_id, char* fpath);

#endif /* end of include guard: WEB_H */
