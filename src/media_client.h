#ifndef MEDIACLIENTBASE_H
#define MEDIACLIENTBASE_H

#include "client_base.h"

class MediaClient : public ClientBase
{
 public:
	MediaClient(ZmdSocket& sock, CLIENT_WORK_MODE mode=CLIENT_WORK_P2P):ClientBase(sock, mode)
	{
	}
   ~MediaClient()
   {}
   void SetClientStatus(MEDIA_CLIENT_STATUS status)
   {
       m_status = status;
   }
   bool  RequestMedia(int chlnum, int streamtype)
   {
       return m_video.RequestMedia(chlnum, streamtype);
   }
   bool  RequestPlayback(const char *filename, int start_time)
   {
       return m_playback.RequestPlayback(filename, start_time);
   }
   void  WaitWorkStop()
   {
       work_imp();
   }
 private:
 	MediaClient();
};

#endif /* end of include guard: P2PCLIENT_H */
