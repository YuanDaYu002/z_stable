#ifndef _ZMD_CLOUD_H_
#define _ZMD_CLOUD_H_

typedef enum 
{
	cloud_video = 2,
	cloud_alarm_video = 4,
}cloud_stream_type;


typedef struct
{
    char identity[64];
    char server_ip[32];
    int  server_port;
	int  change_flag;
}s_transit_server_info;

void cld_set_trans_info(s_transit_server_info * info);

/*
*@ ÔÆ´æ´¢³£×¤Ïß³Ì General iCloud work pthread 
*/
int cloud_init();

#endif
