#include <arpa/inet.h>
#include "device_operation.h"
#include "helpfunction.h"
#include "playback_client.h"
#include "plog.h"

PlaybackClient::PlaybackClient():m_speed_limit(0)
{
}
PlaybackClient::~PlaybackClient()
{
}
bool PlaybackClient::RequestPlayback(const char* filename, int start_time)
{
    m_streamer.Close();
    if(!m_streamer.OpenRecordFile(filename))
    {
        plog("open file (%s) error!!!\n", filename);
        return false;
    }
    m_status = PLAYBACK_SEND_HEAD;
    m_start_time = start_time;
    return true;
}

bool PlaybackClient::RequestPlayback(const char* filename, uint32_t start_time, uint32_t file_start_time, uint32_t file_end_time)
{
    m_streamer.Close();
    if(!m_streamer.OpenRecordFile(filename, file_start_time, file_end_time))
    {
        plog("open file (%s) error!!!\n", filename);
        return false;
    }
    m_status = PLAYBACK_SEND_HEAD;
    m_start_time = start_time;
    return true;
}

bool PlaybackClient::RequestPlayback(uint32_t date, int chl)
{
	plog("date[%u] chl[%d]\n", date, chl);
	if(m_playback_list.size() == 0)
	{
		GetPlaybackList(chl, date);
	}
	for(int i=0; i<m_playback_list.size(); i++)
	{
		if(date >= m_playback_list[i].start_time && date < m_playback_list[i].end_time)
		{
			plog("found file[%s] with date[%u]\n", m_playback_list[i].fpath, date);
			//return RequestPlayback( m_playback_list[i].fpath, date - m_playback_list[i].start_time);
            
			return RequestPlayback( m_playback_list[i].fpath, date - m_playback_list[i].start_time,
                    m_playback_list[i].start_time, m_playback_list[i].end_time);
			break;
		}
	}
	return false;
}

int PlaybackClient::GetData(void**data, int* len)
{
    if(m_status == PLAYBACK_SEND_BODY)
    {
        if(0 != m_streamer.GetNextFrame((const char**)data, len))
        {
            m_status = PLAYBACK_SEND_END;
            return 0;
        }
        return 1;
    }
    else if(m_status == PLAYBACK_SEND_HEAD)
    {
        //todo send head
        if(0 != m_streamer.GetFileHead((const char**)data, len))
        {
            m_status = PLAYBACK_SEND_END;
            return -1;
        }
        if(!m_streamer.MoveTo(m_start_time))
        {
            plog("moveto %d failed!!!\n", m_start_time);
            m_status = PLAYBACK_SEND_END;
            return -1;
        }
        m_status = PLAYBACK_SEND_BODY;
        plog("begin to send file body...\n");
        return 1;
    }
}
void PlaybackClient::ReleasePlayback()
{
    m_streamer.Close();
    m_status = PLAYBACK_SEND_END;
}

const PlaybackClient::p2p_playback_list_t& PlaybackClient::GetPlaybackList(int chl, time_t date)
{
	m_playback_list.clear();

	char str_date[32]="";

	p2p_formattime(&date, str_date, sizeof(str_date));
	// 2015-07-21 00:00:00
	str_date[10]='\0';
	plog("date[%s]\n", str_date);
	p2p_record_info_t *list = NULL;
	int num = 0;
	p2p_get_playback_list_by_date(chl, str_date, &list, &num);

	for(int i=0; i<num; i++)
	{
		m_playback_list.push_back(list[i]);
	}
	free(list);	
	return m_playback_list;
}

const PlaybackClient::p2p_playback_date_list_t 
PlaybackClient::GetPlaybackDateList(int chl, time_t begin, time_t end)
{
    PlaybackClient::p2p_playback_date_list_t date;

    do
    {
        struct tm *t, tm_tmp;
        t = localtime_r(&begin, &tm_tmp);

        //printf("%d-%d-%d\n", t->tm_year+1900, t->tm_mon+1, t->tm_mday);
        begin+=3600*24;
        if(p2p_if_have_record_file_by_date(*t, chl))
        {
            printf("put %d-%d-%d\n", t->tm_year+1900, t->tm_mon+1, t->tm_mday);
            date.push_back(htonl(mktime(t)));
        }

    } while(begin < end + 3600*24);

    return date;
}
