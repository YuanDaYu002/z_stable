#include <vector>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "plog.h"
#include "web.h"
#include "p2p_voice_message.h"
#include "zjson.h"
#include "p2p_interface.h"
#include "device_operation.h"

#define VOICE_MESSAGE_KEEPER_NAME "voice_message_keeper"
extern p2p_init_info_t g_p2p_envir;


VoiceMessageKeeper::VoiceMessageKeeper()
{
	Load();
}

VoiceMessageKeeper::~VoiceMessageKeeper()
{

}

int VoiceMessageKeeper::Parse(const char* json)
{
	int index = json_array_get_length(json);

	if(index < 0) return -1;

	std::vector<p2p_voice_message_info_t> new_msg;

	for(int i = 0; i < index; i++ )
	{	
		char* sub = json_array_get_object_by_idx(json, i);
		if(sub!= NULL)
		{
			JsonParser jp(sub);			
			
			int meida_type = 0;
			jp.GetIntValue(meida_type, "meida_type");
			const char* mid = jp.GetStringConst("id");			
			const char* url = jp.GetStringConst("url");
			if(meida_type == 0 && mid && url)
			{					
				int use = 0; 
				jp.GetIntValue(use,"use");	
				p2p_voice_message_info_t msg = {mid, "", url, use};
				new_msg.push_back(msg);
			}
			free(sub);
		}
	}
	
	/*处理已经删除和修改的记录*/
	for(int j=0; j<P2P_MAX_VOICE_MESSAGE; j++)
	{
		int is_found = 0;
		for(int i=0; i<new_msg.size(); i++)
		{
			if(new_msg[i].mid.size() && new_msg[i].mid == m_voice_message[j].mid)
			{
				is_found = 1;
				m_voice_message[j].use = new_msg[i].use;
			}
		}
		if(!is_found)
		{
			m_voice_message[j].mid = "";
			m_voice_message[j].use = 0;
			unlink(m_voice_message[j].voice_message_path.c_str());
		}
	}
	
	/*处理新增加的记录*/
	for(int i=0; i<new_msg.size(); i++)
	{
		if(HasMid(new_msg[i].mid.c_str()))
			continue;
		for(int j=0; j<P2P_MAX_VOICE_MESSAGE; j++)
		{
			if(m_voice_message[j].mid == std::string(""))
			{
				m_voice_message[j].mid = new_msg[i].mid;
				m_voice_message[j].use = new_msg[i].use;
				//download voice message
				web_get_voicemesage(new_msg[i].url.c_str(), 
					m_voice_message[j].voice_message_path.c_str());
				break;
			}
		}
	}
    if(new_msg.size()==0)
    {
        Clear();
    }

	/*保存*/
	Save();

	return 0;
}
int VoiceMessageKeeper::Load()
{
	char fpath[128] = {0};
	sprintf(fpath, "%s/%s", g_p2p_envir.config_dir, VOICE_MESSAGE_KEEPER_NAME);

    Clear();
	FILE *file = p2p_fopen(fpath, "r");
	if(file)
	{
        while(!feof(file))
        {
            char mid[128]={0};
            char path[128]={0};
            int  use=0;
            fscanf(file, "%[^:]:%[^:]:%d\n", mid, path, &use);
            if(strlen(mid)&& strlen(path))
            {
                FromLoad(mid, path, use);
            }
        }
		fclose(file);
	}
	return 0;
}
int VoiceMessageKeeper::Save()
{
	char fpath[128] = {0};
	sprintf(fpath, "%s/%s", g_p2p_envir.config_dir, VOICE_MESSAGE_KEEPER_NAME);
	
	FILE *file = p2p_fopen(fpath, "w+");
	if(file)
	{
		for(int j=0; j<P2P_MAX_VOICE_MESSAGE; j++)
		{
			if(m_voice_message[j].mid.size())
				fprintf(file, "%s:%s:%d\n", 
					m_voice_message[j].mid.c_str(),
					m_voice_message[j].voice_message_path.c_str(),
					m_voice_message[j].use);
		}
		fclose(file);
	}
	return 0;
}

bool VoiceMessageKeeper::HasMid(const char* mid)
{
	for(int j=0; j<P2P_MAX_VOICE_MESSAGE; j++)
	{
		if(mid == m_voice_message[j].mid)
			return true;
	}
	return false;
}

void VoiceMessageKeeper::FromLoad(const char* mid, const char* path, int use)
{
	for(int j=0; j<P2P_MAX_VOICE_MESSAGE; j++)
	{
		if(path == m_voice_message[j].voice_message_path)
		{
			m_voice_message[j].mid = mid;
			m_voice_message[j].use = use;
		}
	}
}

void VoiceMessageKeeper::Clear()
{
	for(int i=0; i<P2P_MAX_VOICE_MESSAGE; i++)
	{
		m_voice_message[i].voice_message_path = g_p2p_envir.voice_message_path[i];
		m_voice_message[i].use = 0;
		m_voice_message[i].mid = "";
	}
}

const char* VoiceMessageKeeper::GetFilePath(const char* mid)
{
	for(int i=0; i<P2P_MAX_VOICE_MESSAGE; i++)
	{
		if(m_voice_message[i].mid == mid)
			return m_voice_message[i].voice_message_path.c_str();
	}
	return NULL;
}
int VoiceMessageKeeper::GetUseIndex()
{
	/* 找出哪条留言被选中 */
	for(int j=0; j<P2P_MAX_VOICE_MESSAGE; j++)
	{
		if(m_voice_message[j].use== 1)
			return j;
	}	
	/* 默认返回 -1 */
	return -1;
}

