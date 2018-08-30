#ifndef VIDEOCLIENT_H
#define VIDEOCLIENT_H
#include "rtp_spliter.h"

class VideoClient
{
 public:
   VideoClient();
   ~VideoClient();

   bool RequestMedia(int chlnum, int streamtype);
   void RequestAuido(bool on);
   int  GetOneFrame(void** data, int* len);
   void ReleaseMedia();

 private:
   void*  m_mediaid;//media info ptr
   int    m_frameflag;//trans server use this flag to drop frame
   int    m_chlnum;
   bool   m_audio;
   RTPSpliter m_spliter;
};

#endif /* end of include guard: VIDEOCLIENT_H */
