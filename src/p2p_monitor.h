#ifndef _P2P_MONITOR_H_
#define _P2P_MONITOR_H_

typedef struct _p2p_working_status_t {
    time_t start_time;
    time_t last_keep_alive_time;
    int    last_keep_alive_code;
    char   serv_ip[32];
    int    serv_port;
	int    sd_card_status;
	char   status[32];
} p2p_working_status_t;

void p2p_mt_init();
void p2p_mt_set_status(const char* status);
void p2p_mt_dump();
void p2p_mt_set_ac_server_info(const char*serv_ip, int port);
void p2p_mt_set_hearbeat_status(int code);
void p2p_mt_set_sd_card_status(int status);


#endif
