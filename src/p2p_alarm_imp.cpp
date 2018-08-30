#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <vector>
#include <list>
#include <string>
#include <map>
#include <assert.h>

#include "plog.h"
#include "zmdtimer.h"
#include "zmutex.h"

#include "device_operation.h"
#include "helpfunction.h"
#include "p2p_alarm.h"
#include "p2p_streamer.h"
#include "web.h"
#include "web_task.h"
#include "p2p_access_client.h"

#define ALERT_VIDEO_LAST 30
#define MAX_RECORDING_VIDEO 3

extern p2p_init_info_t g_p2p_envir;

typedef struct _passive_alert_record_task {
    //std::string passive_id;
    std::vector<std::string> alert_ids;
    time_t start_time;
    std::string video_file_url;
    int video_file_size;
    int video_last;
    int request_connect_count;
    int connected_count;
} passive_alert_record_task;

struct SendTaskPara
{
    int type;
    int channel;
    int is_main;
    char video_path[128];
    int pic_need_num;
    int pic_get_num;
    std::vector<std::string> pic_paths;
    std::map<std::string, std::string> alert_infos;
    time_t happen_time;
    int  video_last;
    passive_alert_record_task* p_record_task;
};
void p2p_start_record_alarm_video(SendTaskPara *p);
void p2p_send_alert(SendTaskPara* send_task_p);
int passive_connect_record_task(SendTaskPara* p, const char* alert_id);
int passive_start_record(SendTaskPara* p);

static int g_send_video_alert_count = 0;

int handle_new_md_alarm_info(SendTaskPara* p, const char* alarm_info)
{
    //alarm_info: check_type=1/position=1/delta=1 ...
    int pic_count = 0;

    p->alert_infos["type"] = p2p_itoa(p->type);
    p->alert_infos["time"] = p2p_itoa(p->happen_time);
    p->alert_infos["channel"] = p2p_itoa(p->channel);
    p->alert_infos["video_last"] = p2p_itoa(p->video_last);

    const char* pos = alarm_info;

    if(pos && strchr(pos, '=') == 0)
        return 0;
    while(pos && *pos)
    {
        char key[128]="";
        char value[512]="";
        const char* sp = strchr(pos, '/');

        sscanf(pos, "%[^=]=%[^/]/", key, value);
        plog("%s->%s\n", key, value);
        if(strlen(key))
        {
            if(strcmp(key, "pic_count")==0)
                p->pic_need_num = (unsigned int)atoi(value);
            else if(strcmp(key, "video_last")==0)
                p->video_last = (unsigned int)atoi(value);
            else if(strcmp(key, "is_main")==0)
                p->is_main = (unsigned int)atoi(value);

            if(strcmp(key, "image_file")==0)
                p->pic_paths.push_back(std::string("/tmp/")+std::string(value));
            else if(strcmp(key, "video_file")==0)
                strcpy(p->video_path, (std::string("/tmp/")+std::string(value)).c_str());
            else
                p->alert_infos[key] =  value;
        }
        if(sp)
            pos = sp+1;
        else
            break;
    }
    return pic_count;
}

static void capture_proc(SendTaskPara* p)
{	
	char fpath[128];

    if(p->type == P2P_ALARM_IO)
    {
        for(int j=0; j<p2p_get_device_chlnum(); j++)
        {
            if(!p2p_is_channel_io_alarm(p->channel, j))
                continue;
            if(!p2p_get_device_capture(fpath, j, 1))
                continue;
            p->pic_paths.push_back(fpath); 
            p->pic_get_num++;
        }
    }
    else
    {
        if(p2p_get_device_capture(fpath, p->channel, 1))
        {
            p->pic_paths.push_back(fpath); 
            p->pic_get_num++;
        }
        else
        {
            return;
        }
        if(p->pic_get_num>=p->pic_need_num)
        {
            return;
        }
        else
        {
            unsigned long sleep_msec = 600;
            if(p->pic_get_num)
                sleep_msec = 600+(p->pic_get_num-1)*200;
        
            // 0.6 0.8 1.0 1.2s
            plog("usleep %lu\n", sleep_msec);
            usleep(sleep_msec*1000);
            capture_proc(p);
        }
    }
}
void record_task(void* para)
{	
    SendTaskPara *p = (SendTaskPara*)para;
    p2p_start_record_alarm_video(p);	
}
void send_task(void* para)
{
   SendTaskPara *p = (SendTaskPara*)para;

   p2p_send_alert(p);

   if(p2p_get_sync_cvr_on() == 0 && p->video_last > 0)
       g_send_video_alert_count--;
   if(p) delete(p);
}
static void handle_zmodo_alert(int chl, int type, int pic_need_num, int video_last, void* alarm_info)
{
    SendTaskPara *send_task_p = new SendTaskPara;

    send_task_p->pic_need_num = pic_need_num;
    send_task_p->video_last = video_last;
    strcpy(send_task_p->video_path, "");
    send_task_p->pic_get_num=0;
    send_task_p->type = type;
    send_task_p->channel = chl;
    send_task_p->happen_time = time(NULL);
    send_task_p->is_main = 2;   // 2 mean no set value
    handle_new_md_alarm_info(send_task_p, (char*)alarm_info);

    if(p2p_get_sync_cvr_on() > 0 && send_task_p->is_main == 0)
    {
        plog("cloud is on so drop sub alert\n");
		
		/*云存储次告警不传消息 但是还是给应用层一个上报成功的反馈*/
		if(g_p2p_envir.on_alert_report_result)
			g_p2p_envir.on_alert_report_result(1, send_task_p->is_main == 1, send_task_p->type, send_task_p->happen_time);
		
        delete send_task_p;
        send_task_p = NULL;
        return;
    }

    if(p2p_get_sync_cvr_on() == 0 && send_task_p->video_last > 0 && g_send_video_alert_count>=MAX_RECORDING_VIDEO)
    {
        plog("there are 2 task sending video alerts, drop new video...\n");
        send_task_p->video_last = 0;
        send_task_p->alert_infos["video_last"] = "0";
    }
    //开通云存储后不上报视频录像了
    if( p2p_get_sync_cvr_on() == 0 && send_task_p->video_last > 0)
    {
        p2p_long_task_p(record_task, send_task_p);
        g_send_video_alert_count++ ;
    }
    p2p_long_task_p(send_task, send_task_p);
}
static void handle_zmodo_bell(int chl, int type, void* alarm_info)
{
    SendTaskPara *send_task_p = new SendTaskPara;

    send_task_p->pic_need_num = 0;
    send_task_p->pic_get_num=0;
    send_task_p->video_last = 0;
    send_task_p->type = type;
    send_task_p->channel = chl;
    send_task_p->happen_time = time(NULL);
    handle_new_md_alarm_info(send_task_p, (char*)alarm_info);

    if(p2p_get_sync_cvr_on() == 0 && send_task_p->video_last > 0)
    {
        g_send_video_alert_count++ ;
    }
    p2p_long_task_p(send_task, send_task_p);
}

std::list<passive_alert_record_task*> g_passive_record_task_list;
ZMutex g_lock_passive_task;

void passive_send_task(void* para)
{
	char alarm_id[256]="";

	int try_count = 2;

    SendTaskPara *send_task_p = (SendTaskPara*)para;

    if(send_task_p->pic_need_num > 0)
        capture_proc(send_task_p);

	while(try_count > 0)
	{ 
        if(0 == web_upload_alert(send_task_p->alert_infos, alarm_id))
        {
            if(strlen(alarm_id))
                passive_connect_record_task(send_task_p, alarm_id);
            break;
        }

		try_count--;
		sleep(1);
	}
    if(strlen(alarm_id) == 0)
        goto AlertExit;
	try_count=3;
	do
	{	
		if(send_task_p->pic_paths.size())
		{
			if(0  == web_upload_alert_picture(alarm_id, send_task_p->pic_paths))
				break;
		}
		try_count--;
	}while(try_count>0);
AlertExit:

    for(int i=0;i<send_task_p->pic_paths.size(); i++)
    {
        unlink(send_task_p->pic_paths[i].c_str());
    }
    delete send_task_p;
	plog("send alert done!\n");
}

void passive_record_task(void* para)
{
    SendTaskPara *p = (SendTaskPara*)para;

    SendTaskPara task = *p;

    g_send_video_alert_count++;
    p2p_start_record_alarm_video(&task);	

    char file_url[1024]="";
    
	int try_count = 3;

	while(try_count > 0)
	{ 
        if(0 == web_upload_file(task.video_path, 0, file_url))
            break;

		try_count--;
		sleep(3);
	}
    if(try_count > 0)
    {
        std::string ids;
        int wait_count = 0;
        while(task.p_record_task->request_connect_count != task.p_record_task->connected_count)
        {
            if(wait_count++ >= 10)
                break;
            plog("waitting unfinish passive alert msg [%d-%d]....\n", 
                    task.p_record_task->request_connect_count, task.p_record_task->connected_count);
            sleep(1);
        }
        for(int i=0; i< task.p_record_task->alert_ids.size(); i++)
        {
            if(ids.size())
                ids+= "-";
            ids+= task.p_record_task->alert_ids[i];
        }
        if(ids.size())
        {
            try_count = 3;

            while(try_count > 0)
            { 
                if(0 == web_update_alert(ids.c_str(), file_url, get_filesize(task.video_path), task.video_last))
                    break;
                try_count--;
                sleep(3);
            }
        }
    }
    if(task.p_record_task)
    {
        ZMutexLock l(&g_lock_passive_task);
        std::list<passive_alert_record_task*>::iterator it;
        for(it = g_passive_record_task_list.begin(); it != g_passive_record_task_list.end(); it++)
        {
            if(*it == task.p_record_task)
            {
                plog("release one record task\n");
                g_passive_record_task_list.erase(it);
                break;
            }
        }
        delete task.p_record_task;
    }
    unlink(task.video_path);
    g_send_video_alert_count--;
}

static void handle_zmodo_doormegnet(int chl, int type, void* alarm_info)
{
    SendTaskPara *send_task_p = new SendTaskPara;

    send_task_p->pic_need_num = 1;
    send_task_p->pic_get_num=0;
    send_task_p->video_last = p2p_base()->dm_record_time;
    send_task_p->type = type;
    send_task_p->channel = chl;
    send_task_p->happen_time = time(NULL);
    handle_new_md_alarm_info(send_task_p, (char*)alarm_info);

    //开通云存储后不上报视频录像了
    if( p2p_get_sync_cvr_on() == 0 && send_task_p->video_last > 0)
        passive_start_record(send_task_p);
    p2p_long_task_p(passive_send_task, send_task_p);
}

void handle_alarm()
{
    BlockWaitForAlarms();

	p2p_broadcast_alarm_t alarm = {0,0,NULL};

    int ret = P2pGetOneAlarm(&alarm);

	if(!p2p_is_device_online() || ret != 0)
    {
        if(alarm.alarm_info)
            free(alarm.alarm_info);
        plog("device offline, drop alarm...\n");
		return;
    }

    switch(alarm.alarm_type)
    {
        case P2P_ALARM_MD:
        case P2P_ALARM_PIR:
            handle_zmodo_alert(alarm.channel, alarm.alarm_type, 1, 0, alarm.alarm_info);
            break;
#if 0
        case P2P_ALARM_AUDIO_EXCEPTION:
        case P2P_ALARM_BABY_CRY:
            handle_one_picture_alarm(alarm.channel, alarm.alarm_type);
            break;
#endif
        case P2P_ALARM_IO:
            handle_zmodo_alert(alarm.channel, alarm.alarm_type, 1, 0, alarm.alarm_info);
            break;

        case P2P_ALARM_MICROWAVE:
            handle_zmodo_alert(alarm.channel, alarm.alarm_type, 1, 30, alarm.alarm_info);
            break;
        case P2P_ALARM_DOORMEGNET_CLOSE:
        case P2P_ALARM_DOORMEGNET_OPEN:
            handle_zmodo_doormegnet(alarm.channel, alarm.alarm_type, alarm.alarm_info);
            break;
        case P2P_ALARM_VIDEOLOST:
        case P2P_ALARM_SD_EXCEPTION:
		case P2P_ALARM_BABY_CRY:
        case P2P_ALARM_SMOG:
        case P2P_ALARM_COMBUSTIBLE_GAS:
        case P2P_ALARM_EMERGENCY_BUTTON:
        case P2P_ALARM_HOME_MODE:
        case P2P_ALARM_OUT_MODE:
        case P2P_ALARM_SLEEP_MODE:
        case P2P_ALARM_CUSTOM_MODE:
        case P2P_ALARM_BUZZER_TRIGGER:
        case P2P_ALARM_BUZZER_HALT:
        case P2P_ALARM_REMOTE_CONTROL:
        case P2P_ALARM_BELL_RING:
            handle_zmodo_alert(alarm.channel, alarm.alarm_type, 0, 0, alarm.alarm_info);
            break;
        case P2P_ALARM_ANSWER:
        case P2P_ALARM_NO_ANSWER:
            handle_zmodo_bell(alarm.channel, alarm.alarm_type, alarm.alarm_info);
            break;
        default:
            plog("unknow alarm type [%d]!!!\n", alarm.alarm_type);
            break;
    }
    if(alarm.alarm_info)
        free(alarm.alarm_info);
}


//send alarm pic and cover pic
void handle_alarm_thread(void* parm)
{
    plogfn();

    while( true )
    {
        handle_alarm();
		usleep( 100*1000 ) ;        
    }   
}
int start_alarm_handler()
{
	p2p_long_task(handle_alarm_thread);
    return 0;
}

void p2p_start_record_alarm_video(SendTaskPara *p)
{
    int if_video_last_get = 0;

    char tmp[32] = "/tmp/alarm_video_cache_XXXXXX";

    int fd = mkstemp(tmp);

    close(fd);

    FILE* file = fopen(tmp, "wb");

    if(!file)
    {
        plog("create file %s error!\n" , tmp);
        return;
    }

    strcpy(p->video_path, tmp);

    int begin_time = p2p_get_uptime();
    void* mid = p2p_get_media_id(0, 1);
    //p2p_set_current_i_frame(mid);
    p2p_set_i_frame_by_second(mid);
    int video_begin_sec = 0;
    int video_current_sec = 0;
    int video_last = p->video_last;
    int test_count = 0;
    while(1)
    {
        unsigned char* data = 0 ;
        FrameInfo info={0} ;
        int getRet = 0 ;

        getRet = p2p_get_one_frame(mid, &data , &info );
        if( !getRet && data && info.FrmLength > 0 )
        {
            if( info.FrmLength >= MAX_FRAME_SIZ)
            {
                plog("frame to big!!! (%lu)\n", info.FrmLength);
                p2p_free_media_id(mid);
                return ;
            }

            VideoFrameHeader* fh = (VideoFrameHeader*)data;;

            if(fh->m_nVHeaderFlag != IFRAME_FLAG && 
                    fh->m_nVHeaderFlag != PFRAME_FLAG &&
                    fh->m_nVHeaderFlag != AFRAME_FLAG)
            {
                plog("invalid header flag:[0x%x]\n", fh->m_nVHeaderFlag);
                p2p_set_i_frame(mid);
                continue;
            }
            if(fh->m_nVHeaderFlag == IFRAME_FLAG || fh->m_nVHeaderFlag == PFRAME_FLAG)
            {
                video_current_sec = (int)(fh->m_lVPts/(long long)(1000*1000));
                if(video_begin_sec == 0)
                    video_begin_sec = video_current_sec;
            }
            test_count++;
            fwrite(data, 1, info.FrmLength, file);
            fflush(file);
        }
        else
        {
            if(p2p_get_uptime() - begin_time >= video_last /*|| video_current_sec - video_begin_sec >= video_last*/)
                break;
            if(getRet == -2)
                continue;
            if(if_video_last_get == 0)
            {
                p->video_last += video_current_sec-video_begin_sec;
                if_video_last_get = 1;
                plog_int(p->video_last);
                plog_int(video_begin_sec);
                plog_int(video_current_sec);
                plog_int(test_count);
            }
            usleep(10*1000);
        }
    }
    p2p_free_media_id(mid);
    fclose(file);
    plog("done!\n");
}
void p2p_send_alert(SendTaskPara* send_task_p)
{
    char video_file_url[1024]="";
    int  video_file_size = 0;
	char alarm_id[256]="";
	char fname[128]="";

	int try_count = 3;
	int alert_upload_done = 0;

    if(send_task_p->pic_need_num > 0)
        capture_proc(send_task_p);

	while(try_count > 0)
	{ 
        int ret = web_upload_alert(send_task_p->alert_infos, alarm_id);

		try_count--;
        if(ret == 0 || ret == 1 || try_count <= 0)
        {
            if(g_p2p_envir.on_alert_report_result)
            {
                //当设备没有绑定时，重传前等待下
                if(send_task_p->is_main == 1)
                    sleep(3);
                g_p2p_envir.on_alert_report_result(ret == 0, send_task_p->is_main == 1, send_task_p->type, send_task_p->happen_time);
            }
            break;
        }
		sleep(3);
	}

	alert_upload_done = strlen(alarm_id)>0;

    if(alert_upload_done == 0)
    {
        // alert upload failure
        goto AlertExit;
    }

	try_count=3;
	do
	{	
		if(send_task_p->pic_paths.size())
		{
			if(0  == web_upload_alert_picture(alarm_id, send_task_p->pic_paths))
				break;
		}
		try_count--;
	}while(try_count>0);
    
    if(send_task_p->video_last <= 0)
        goto AlertExit;
	try_count = 3;
	sprintf(fname, "alert_%d_%s.h264", send_task_p->type, p2p_get_deviceid());

    //wait for video file create done
    sleep(1);

	do
	{
        if(p2p_get_sync_cvr_on() != 0)
        {
            plog("video cloud is on, so drop video!\n");
            break;
        }
		if(strlen(send_task_p->video_path))
        {
            if(0 == web_upload_alert_video(alarm_id, send_task_p->video_path, fname,
                        send_task_p->video_last, video_file_url, &video_file_size))
                break;
        }
		else
		{
			plog("record error!\n");
			break;
		}
		try_count--;
	}while(try_count>0);

AlertExit:
    if(alert_upload_done == 0 && send_task_p->video_last > 0)
    {
        //wait for video file create done 
        sleep(2);
    }
	if(strlen(send_task_p->video_path)) unlink(send_task_p->video_path);

    for(int i=0;i<send_task_p->pic_paths.size(); i++)
    {
        unlink(send_task_p->pic_paths[i].c_str());
    }
	plog("send alert done!\n");
}

void test_alert_record()
{
    SendTaskPara p;

    memset(&p , 0, sizeof(p));
    p.video_last = 10;
    p2p_start_record_alarm_video(&p);
    if(strlen(p.video_path))
        unlink(p.video_path);

}
//for outter use
int p2p_record_stream(char* video_file, int* video_last)
{
    SendTaskPara p;

    memset(p.video_path, 0, sizeof(p.video_path));
    p.video_last = *video_last;

    p2p_start_record_alarm_video(&p);	

    *video_last = p.video_last;
    strcpy(video_file, p.video_path);

    return 0;
}

int passive_start_record(SendTaskPara* p)
{
    int need_start_record = 1;

    ZMutexLock l(&g_lock_passive_task);
    
    std::list<passive_alert_record_task*>::iterator it;

    for(it = g_passive_record_task_list.begin(); it != g_passive_record_task_list.end(); it++)
    {
        plog_int(p->happen_time);
        plog_int((*it)->start_time);
        plog_int(p->video_last);
        if(p->happen_time - (*it)->start_time < p->video_last*0.75)
        {
            need_start_record = 0;
            (*it)->request_connect_count++;
            return 0;
        }
    }
    if(need_start_record)
    {
        plog("start a new record task\n");
        passive_alert_record_task *task = new passive_alert_record_task;

        task->start_time = p->happen_time;
        task->video_last = p->video_last;
        task->request_connect_count = 1;
        task->connected_count = 0;

        g_passive_record_task_list.push_back(task);

        p->p_record_task = task;

        p2p_long_task_p(passive_record_task, p);
    }
    return 0;
}

int passive_connect_record_task(SendTaskPara* p, const char* alert_id)
{
    ZMutexLock l(&g_lock_passive_task);
    std::list<passive_alert_record_task*>::reverse_iterator it;

    for(it = g_passive_record_task_list.rbegin(); it != g_passive_record_task_list.rend(); it++)
    {
        if(p->happen_time - (*it)->start_time < p->video_last*0.75)
        {
            (*it)->alert_ids.push_back(alert_id);
            (*it)->connected_count++;
            plog("connect with a record task\n");
            return 0;
        }
    }
    plog("failed to connect with a record task!!!\n");
    return 0;
}

