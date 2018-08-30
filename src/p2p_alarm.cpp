#if 0
#include <vector>
#include <string>
#include <assert.h>
#include <map>

#include "plog.h"
#include "p2p_alarm.h"
#include "web_task.h"
#include "web.h"
#include "helpfunction.h"
#include "device_operation.h"
#include "test_config.h"
#include "zmdtimer.h"
#include "zmd_task.h"

static void on_alarm_done(int result, const std::string& resp, void* para);

/**
 *  
 * @brief AlarmPoster的具体实现
 * 
 * @author AlbertXiao (2015/5/19)
 */
class AlarmPosterImp: public AlarmPoster
{ 
 public:
	//channel 是视频或者IO通道号
	void CreateAlarm(P2pAlarmType type, time_t happen_time , int channel)
	{
		m_type = type;
		m_channel = channel;
		m_happen_time = happen_time;
	}
	//channel 是视频通道号
	void AddPicture(const char* file_path, int channel, int need_delete)
	{
		AlarmPosterImp::PictureInfo info = {file_path, channel, need_delete};
		m_files.push_back(info);		
	}

	void AddSubDeviceID(const char* id)
	{
		P2P_ASSERT(id!=NULL);
		m_sub_device_id = id;
	}
	void AddRecord(const char* file_path, uint32_t file_size)
	{
		m_video_file_path = file_path;
		m_video_file_size = file_size;
	}
	void AddString(const char* name, const char* value)
	{
		if(name && value)
			m_strings[name] = value;
	}
	void FillAlarmBody(WebTask *task)
	{		
        std::string url;

		url = get_alerts_server();
		url += WEB_ALARM_URI;
		
		task->SetUrl(url.c_str());
		task->AddPostString("tokenid", web_get_tokenid());
		task->AddPostString("physical_id", p2p_get_deviceid());
		task->AddPostString("type", p2p_itoa(m_type));
		task->AddPostString("time", p2p_utoa(m_happen_time));
		task->AddPostString("channel", p2p_itoa(m_channel));
		if(m_sub_device_id.size())
			task->AddPostString("passive_id", m_sub_device_id.c_str());
		
		for(int i = 0; i < m_files.size(); i++)
		{
			task->AddPostPicture("image_name[]",
						m_files[i].file_path.c_str(),
						GetFormatedName(m_files[i].file_path.c_str(),
										 m_files[i].channel, m_happen_time+i).c_str());
		}
		if(m_video_file_path.size())
		{
			task->AddPostString("video_file_name", m_video_file_path.c_str());
			task->AddPostString("video_file_size", p2p_utoa(m_video_file_size/1024));			
		}
		std::map<std::string, std::string>::iterator iter;

		for(iter = m_strings.begin(); iter != m_strings.end(); iter++)
		{
			task->AddPostString(iter->first.c_str(), iter->second.c_str());
		}
		task->DoGetString();
	}
	int PostAlarm()
	{		
		//just try 5 times
		if(m_try_count++ > 4)
			return -1;
	 	WebTask *task = new WebTask;

		FillAlarmBody(task);
		task->SetCallback(on_alarm_done, this);
		//ZmdCreateTimer(1000*(m_try_count*2-1), _task_worker, task);
		if(m_try_count>1)
			ZmdCreateDelayTask(1000*(m_try_count-1)*2, _task_worker, task, "alarm poster");
		else
			ZmdCreateTask(_task_worker, task, "alarm poster");

		return 0;
	}
	int SendAlarm()
	{		
		WebTask *task = new WebTask;
		int ret = -1;

		FillAlarmBody(task);
		if(!task->WaitTaskDone())
		{
			plog("recv:%s\n", task->GetResultString());
			if(!web_check_result(task->GetResultString()))
				ret = 0;
		}
		delete task;
		return ret;
	}

	void DeletePictures()
	{
		for(int i = 0; i < m_files.size(); i++)
		{
			if( m_files[i].need_delete )
				unlink(m_files[i].file_path.c_str());
		}
	}

	AlarmPosterImp():m_try_count(0)
	{
	}
	~AlarmPosterImp(){}

 private:
 	std::string GetFormatedName(const char* file_path, int channel, time_t happen_time)
 	{
		std::string name = "";
		char date[32];
		
		switch(m_type)
		{
		case P2P_ALARM_MD:
        case P2P_ALARM_MICROWAVE:
			name = "md_";
			break;
		case P2P_ALARM_IO:
			name = "io_";
			break;
		case P2P_ALARM_AUDIO_EXCEPTION:
			name = "ep_";
			break;
		case P2P_ALARM_BABY_CRY:
			name = "babyCry_";
			break;
		case P2P_ALARM_DOORMEGNET_OPEN:
		case P2P_ALARM_DOORMEGNET_CLOSE:
			name = "dc_";
			break;
		default:
			plog("unknow type(%d) for formate name!!!\n", m_type);
			break;
		}
		name += p2p_itoa(channel);
		name += "_";
		name += get_date4capture(date, sizeof(date), &happen_time);
		name += ".jpg";
		return name;
 	}
	static void _task_worker(void* para)
	{
		WebTask *task = reinterpret_cast<WebTask*>(para);
		task->WaitTaskDone();
		delete task;
	}

 private:
 	struct PictureInfo
	{
		std::string file_path;
		int channel;
		int need_delete;
	};
 private:
	P2pAlarmType m_type;
	int 		 m_channel;
	time_t       m_happen_time;
	std::vector<AlarmPosterImp::PictureInfo> m_files;
	std::string  m_video_file_path;
	uint32_t	 m_video_file_size;
	int          m_try_count;
	std::string  m_sub_device_id;
	std::map<std::string, std::string> m_strings;
};


AlarmPoster* AlarmPoster::CreateAlarm(P2pAlarmType type, time_t happen_time , int channel )
{
	if(!p2p_is_online())
	{
		printf("AlarmPoster::CreateAlarm error [p2p is offline]!!!\n");
		return NULL;
	}
	AlarmPosterImp *poster = new AlarmPosterImp;
	
	poster->CreateAlarm(type,happen_time,channel);
	return poster;
}
void AlarmPoster::AddSubDeviceID(AlarmPoster *poster, const char* id)
{
	if(poster)
		reinterpret_cast<AlarmPosterImp*>(poster)->AddSubDeviceID(id);
}

//channel 是视频通道号
void AlarmPoster::AddPicture(AlarmPoster *poster, 
				const char* file_path, int channel, int need_delete )
{
	if(poster)
		reinterpret_cast<AlarmPosterImp*>(poster)->AddPicture(file_path, channel, need_delete);
}

void AlarmPoster::AddRecord(AlarmPoster *poster, const char* file_path, uint32_t file_size)
{
	if(poster)
		reinterpret_cast<AlarmPosterImp*>(poster)->AddRecord(file_path, file_size);
}

void AlarmPoster::AddString(AlarmPoster *poster, const char* name, const char* value)
{
	if(poster)
		reinterpret_cast<AlarmPosterImp*>(poster)->AddString(name, value);
}

void AlarmPoster::PostAlarm(AlarmPoster *poster)
{	
	if(poster)
		reinterpret_cast<AlarmPosterImp*>(poster)->PostAlarm();
}

int AlarmPoster::SendAlarm(AlarmPoster *poster)
{
	int ret = -1;
	if(poster)
	{
		ret = reinterpret_cast<AlarmPosterImp*>(poster)->SendAlarm();
		delete reinterpret_cast<AlarmPosterImp*>(poster);
	}
	return ret;
}

static void on_alarm_done(int result, const std::string& resp, void* para)
{	
	AlarmPosterImp * imp = (AlarmPosterImp*)para;

	plog("post alarm resp:%s\n", resp.c_str());
	if( result != 0 || web_check_result(resp.c_str()) != 0)
	{
		if(!p2p_is_device_online())
		{
			plog("device is offline, stop post alarm!\n");
			goto END;
		}
		if(imp->PostAlarm() != 0)
		{
			plog("failed to post alarm!\n");
			goto END;
		}
		return;
	}
	
END:
	imp->DeletePictures();
	delete imp;
}
#endif
