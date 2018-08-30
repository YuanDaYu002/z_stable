#include <stdio.h>
#include <time.h>
#include <string.h>

#include "zmdtimer.h"
#include "p2p_monitor.h"
#include "device_operation.h"
#include "helpfunction.h"

static p2p_working_status_t working_status;


static void working_output_timer_func(void* user_data)
{
	p2p_mt_dump();
	ZmdCreateShortTimer(10000, working_output_timer_func, NULL );
}

void p2p_mt_init()
{
    working_status.start_time = time(NULL);
    strcpy(working_status.serv_ip, "");
    working_status.serv_port = 0;
    working_status.last_keep_alive_time = 0;
    working_status.last_keep_alive_code = 0;
	working_status.sd_card_status = 4;

	working_output_timer_func(NULL);
}
void p2p_mt_set_ac_server_info(const char*serv_ip, int port)
{
	strcpy(working_status.serv_ip, serv_ip);
    working_status.serv_port = port;
}

void p2p_mt_set_status(const char* status)
{
	strcpy(working_status.status, status);
}

#define P2P_MT_PRINT_SPLITER \
	fprintf(file, "%s\n", "---------------------------------------")
void p2p_mt_dump()
{
    FILE *file = p2p_fopen("/tmp/p2p_working_status", "w");
    if(file)
    {
        char time1[32], time2[32];
		fprintf(file, P2P_COLOR_GREEN);
		P2P_MT_PRINT_SPLITER;
		fprintf(file, "build date: [%s %s]\n", __DATE__, __TIME__);
		fprintf(file, "svn version: [%s]\n",P2P_LIB_VERSION);
		P2P_MT_PRINT_SPLITER;
		fprintf(file, "device id: [%s]\n", p2p_get_deviceid());
		fprintf(file, "%s[%u]\n", "device capacity: ", p2p_get_device_capacity());
		fprintf(file, "%s[%u]\n", "device extend capacity: ", p2p_get_device_extend_capacity());
		P2P_MT_PRINT_SPLITER;
		fprintf(file, "working status: [%s]\n", working_status.status);
		P2P_MT_PRINT_SPLITER;
        fprintf(file, "start time\t\tlast heart beat\t\tcode\tserver ip\tserver port\n");
        fprintf(file, "%s\t%s\t%d\t%s\t%d\n",
                p2p_formattime(&working_status.start_time, time1, 32),
                p2p_formattime(&working_status.last_keep_alive_time, time2, 32),
                working_status.last_keep_alive_code,
                working_status.serv_ip,
                working_status.serv_port);

        P2P_MT_PRINT_SPLITER;

		static char *sd_card_stats_string[] = {"OK", "not exist", "not mount", "formating", "initing"};
		fprintf(file, "%s", "sd_card status:");
		fprintf(file, "[%s]\n", sd_card_stats_string[working_status.sd_card_status]);
		P2P_MT_PRINT_SPLITER;

		fprintf(file, P2P_COLOR_NONE);
        fclose(file);
    }
}
void p2p_mt_set_hearbeat_status(int code)
{
    working_status.last_keep_alive_time = time(NULL);
    working_status.last_keep_alive_code = code;
}
void p2p_mt_set_sd_card_status(int status)
{
	working_status.sd_card_status = status;
}

