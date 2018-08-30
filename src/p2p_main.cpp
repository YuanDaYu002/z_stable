#include <unistd.h>
#include <time.h>
#include "web.h"
#include "plog.h"
#include "device_operation.h"
#include "helpfunction.h"
#include "zmdtimer.h"
#include "zmd_cloud.h"
#include "p2p_monitor.h"
#include "p2p_def.h"
#include "p2p_access_client.h"

p2p_base_t g_session;
extern int start_alarm_handler();

void sync_paramter(int delay_sec);

p2p_base_t* p2p_base()
{
    return &g_session;
}
static void web_login_timer_func(void* user_data)
{		
    //for thread safe
    if(P2P_CHECK_STATUS(P2P_LOGINING))
        return;

    plog("%s\n", "login again!!!");
    P2P_SET_STATUS(P2P_LOGINING);
    if(!web_login(&g_session, 10))
    {
        //for smartlink
        if(p2p_get_smartlink_mark())
        {				
            p2p_broadcast_device();
            web_report_info(p2p_get_deviceid(), web_get_tokenid(), 3);
            p2p_clear_smartlink_mark();
            p2p_send_cover_pic(0);
        }

        //p2p_mt_set_ac_server_info(devconn_ip, devconn_port);
        if(p2p_access_client_if_stop())
        {
            plog("restart access client!\n");
            p2p_access_client_start(&g_session);
        }
    }
    else
    {
        P2P_SET_STATUS(P2P_NEED_LOGIN);

        ZmdCreateTimer(p2p_get_rand(10, 15000), web_login_timer_func, NULL);
    }
}

void sync_paramter_timer_func(void* user_data)
{
    int try_times = (int)user_data;

    if(0 != web_sync_paramter())
    {
        if(--try_times < 0)
        {
            plog("sync failed for n times ,end!\n");
            return;
        }
        ZmdCreateTimer(1000, sync_paramter_timer_func,(void*)try_times);	
    }
}
void sync_paramter(int delay_sec)
{
    ZmdCreateTimer(1000*delay_sec, sync_paramter_timer_func, (void*)3);	
}

void relogin_web(int delay_sec)
{
    ZmdCreateTimer(1000*delay_sec, web_login_timer_func, NULL);
}

//global session operation
void init_global_session()
{
    P2P_SET_STATUS(P2P_NEED_LOGIN);
    g_session.server_timestamp_base_uptime = -1;
    strcpy(g_session.device_id, p2p_get_deviceid());
    g_session.cover_refresh_time = p2p_get_rand(4*3600, 6*3600);
    g_session.dm_record_time = 30;
    g_session.cvr_timeout_intv = 10;
    g_session.cloud_stream_type = 2;
    g_session.cloud_iframe_interval = 10;
    g_session.token_valid = 0;
    g_session.rtsp_frame_rate = 10;
#ifdef AMBA
    g_session.rtsp_bit_rate = 300;
#else
    g_session.rtsp_bit_rate = 200;
#endif
}

bool p2p_is_device_online()
{
    return p2p_wifi_connected() && P2P_CHECK_STATUS(P2P_ONLINE);
}

static void upnp_report_timer_func(void* user_data)
{
    static int  last_upnp_video_port = -1;
    static char last_local_ip[32];
    static char last_device_mac[32];
    static char last_net_mask[32];
    static char last_gateway_ip[32];
    static char last_gateway_mac[32];
    static char last_wifi_ssid[32];

    int  upnp_video_port;
    char local_ip[32]={0};
    char device_mac[32]={0};
    char net_mask[32]={0};
    char gateway_ip[32]={0};
    char gateway_mac[32]={0};
    char wifi_ssid[32]={0};
    char wifi_pwd[32]={0};
    int  report_code = 200;
    int  delay = 1000*60;

    upnp_video_port = p2p_get_upnp_video_port();

    p2p_get_wifi_info(wifi_ssid, wifi_pwd);

    p2p_get_local_net_info(p2p_get_network_interface(), local_ip, net_mask, device_mac);
    p2p_get_gateway_info(p2p_get_network_interface(), gateway_ip, gateway_mac);

    if(upnp_video_port != last_upnp_video_port ||
            strcmp(local_ip, last_local_ip) ||
            strcmp(device_mac, last_device_mac) ||
            strcmp(net_mask, last_net_mask) ||
            strcmp(gateway_ip, last_gateway_ip) ||
            strcmp(gateway_mac, last_gateway_mac) ||
            strcmp(wifi_ssid, last_wifi_ssid) )
    {
        report_code = web_report_upnp(web_get_tokenid(), 
                upnp_video_port,
                p2p_get_local_video_port(),
                device_mac,
                local_ip,
                net_mask,
                gateway_mac,
                gateway_ip,
                wifi_ssid);

        if(report_code == 0) 
        {
            last_upnp_video_port = upnp_video_port;
            strcpy(last_local_ip, local_ip);
            strcpy(last_device_mac, device_mac);
            strcpy(last_net_mask, net_mask);
            strcpy(last_gateway_ip, gateway_ip);
            strcpy(last_gateway_mac, gateway_mac);
            strcpy(last_wifi_ssid, wifi_ssid);
            report_code = 200;
            p2p_on_device_online(1);
        }
    }
    ZmdCreateTimer(delay, upnp_report_timer_func, NULL );

}
static void send_cover_picture_timer_func(void* user_data)
{
    int delay=1000*p2p_get_rand(60, 8*3600);	
    int channel = (int)user_data;

    if(P2P_CHECK_STATUS(P2P_ONLINE) && 0 == p2p_send_cover_pic(channel))
    {
        delay = p2p_base()->cover_refresh_time * 1000;
        if(delay < 5*60*1000)
            delay = p2p_get_rand(4*3600, 6*3600)*1000;
    }

    ZmdCreateTimer(delay, send_cover_picture_timer_func, (void*)channel );	
}
static void get_timezone_timer_func(void* user_data)
{
    int delay=p2p_get_rand(8*3600, 10*3600)*1000;

    if(p2p_is_ntp_enable())
    {
        //if fail try again
        if(web_sync_paramter())
            delay = p2p_get_rand(50, 3600)*1000;
    }
    ZmdCreateTimer(delay, get_timezone_timer_func, NULL );	
}

static void check_sd_card_timer_func(void* user_data)
{
    static int reported = 0;
    int status = 0;
    SD_CARD_STAT st;

    p2p_get_sd_card_status(&st);
    status = st.status;

    p2p_mt_set_sd_card_status(status);

    if(status == 2 && !reported)
    {
#if 0
        AlarmPoster *poster = AlarmPoster::CreateAlarm(P2P_ALARM_SD_EXCEPTION);
        if(poster) 
        {
            reported = !AlarmPoster::SendAlarm(poster);
        }
#endif
    }
    //ready for next report
    if(status != 2 && reported)
        reported = 0;

    ZmdCreateTimer(1000*3600, check_sd_card_timer_func, NULL );	
}

void init_timers()
{
    ZmdCreateTimer(/*5000*/ 0, upnp_report_timer_func, NULL );

#ifndef _BUILD_FOR_NVR_
    //ZmdCreateTimer(30000, check_sd_card_timer_func , NULL);
    // use a timer to make sure picture send success
    //for(int i=0; i<p2p_get_device_chlnum(); i++)
    ZmdCreateTimer(1000, send_cover_picture_timer_func, (void*)0 );
#endif


    ZmdCreateTimer(8*3600*1000, get_timezone_timer_func, NULL );
}
int ac_network_cb(access_network_status_t status)
{
    plog_int(status);
    static int ac_relogin_count = 0;

    if(status == ACCESS_NETWORK_LOGIN_DONE)
    {
        P2P_SET_STATUS(P2P_ONLINE);
        p2p_handle_timing(p2p_access_client_get_server_timestamp());
        ac_relogin_count = 0;
        /* if data change when device was offline*/
        sync_paramter(p2p_get_rand(1,60*10));
    }
    else if(status == ACCESS_NETWORK_STOP)
    {
        plog("access client is not running!\n");
        P2P_SET_STATUS(P2P_HEARTBEAT_ERROR);

        if(!P2P_CHECK_STATUS(P2P_LOGINING) && !P2P_CHECK_STATUS(P2P_NEED_LOGIN))
        {
            P2P_SET_STATUS(P2P_NEED_LOGIN);

            ZmdCreateTimer(1000*p2p_get_rand(5, 20), web_login_timer_func, NULL);
        }
    }
    else if(status == ACCESS_NETWORK_HEARTBEAT_ERROR)
    {
        P2P_SET_STATUS(P2P_HEARTBEAT_ERROR);
    }
    else if(status == ACCESS_NETWORK_RELOGIN)
    {
        plog("access client is not running!\n");
        P2P_SET_STATUS(P2P_HEARTBEAT_ERROR);
        if(ac_relogin_count++ < 3)
        {
            plog("restart access client to do relogin!\n");
            usleep(p2p_get_rand(100, 5000));
            p2p_access_client_start(&g_session);
        }
        else
        {
            ac_relogin_count = 0;
            ac_network_cb(ACCESS_NETWORK_STOP);
        }
    }
    return 0;
}

static void meshare_init_timer(void* user_data)
{	 
    plogfn();

    //init web
    web_init();

    init_global_session();

    p2p_mt_init();

    /* remove stun info */
    p2p_remove_dir(STUN_DIR);

    /* wait for network ready */
    while(!p2p_network_ok())
    {
        P2P_SET_STATUS(P2P_NETWORK_NOT_READY);
        sleep(1);
    }

    sleep(2);
    P2P_SET_STATUS(P2P_LOGINING);

    int login_timeout = 3;
    while( 0 != web_login(&g_session, 10) )
    {
        login_timeout+=2;
        if(login_timeout > 10)
            login_timeout = 3;
        sleep(1);
    }

    //report info
    int try_count = 5;

    // if this is the first run, report must be done!
    if(p2p_is_restored())
        try_count = 10000;

    int report_timeout=3;
    while(0!=web_report_info(p2p_get_deviceid(), web_get_tokenid(), report_timeout)&& try_count > 0)
    {
        try_count--;
        report_timeout+=2;
        if(report_timeout > 10)
            report_timeout = 3;
        sleep(1);
    }

    //for smartlink
    if(p2p_get_smartlink_mark())
    {			
        p2p_broadcast_device();
        p2p_clear_smartlink_mark();
    }

    //plog("server info:%s:%d\n", g_session.access_ip, g_session.access_port);

    /* start access client */
    p2p_access_client_init();

    p2p_access_client_set_callback(ac_msg_handler, ac_network_cb);

    p2p_access_client_start(&g_session);

    //p2p_mt_set_ac_server_info(g_session.access_ip, g_session.access_port);

    sync_paramter(0);

    init_timers();

    start_alarm_handler();

    /* iCloud work pthread init */
    cloud_init();

    if(p2p_is_boot_after_upgrade())
        p2p_upload_upgrade_success();
}

void test_streamer();
int p2p_main()
{	
    ZmdCreateTimer(0, meshare_init_timer, NULL); 

    /* test streamer in local network*/
    if(getenv("P2P_TEST_STREAMER"))
        test_streamer();

    return 0;
}

