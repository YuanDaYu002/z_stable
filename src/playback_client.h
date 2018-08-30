#ifndef PLAYBACKCLIENT_H

#define PLAYBACKCLIENT_H
#include <vector>
#include <time.h>
#include <stdint.h>
#include "zmd_msg.h"

#include "record_streamer.h"

class PlaybackClient
{
 public:
typedef enum {
    PLAYBACK_SEND_END=0,
    PLAYBACK_SEND_HEAD,
    PLAYBACK_SEND_BODY
}playback_status_t;
typedef std::vector<p2p_record_info_t> p2p_playback_list_t;
typedef std::vector<int> p2p_playback_date_list_t;

 public:
   PlaybackClient();
   ~PlaybackClient();

   bool RequestPlayback(const char* filename, int start_point);
   //for nvr
   bool RequestPlayback(const char* filename, uint32_t start_time, uint32_t file_start_time, uint32_t file_end_time);

   bool RequestPlayback(uint32_t date, int chl);
   int  GetData(void**data, int* len);
   void ReleasePlayback();
	//get playback list by date 
   const p2p_playback_list_t& GetPlaybackList(int chl, time_t date);
   const p2p_playback_date_list_t GetPlaybackDateList(int chl, time_t begin, time_t end);
 private:
   RecordStreamer       m_streamer;
   playback_status_t    m_status;
   int                  m_start_time;
   unsigned int			m_speed_limit;
   p2p_playback_list_t  m_playback_list;

};

#endif /* end of include guard: PLAYBACKCLIENT_H */
