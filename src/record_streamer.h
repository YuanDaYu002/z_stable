#ifndef RECORDSTREAMER_H
#define RECORDSTREAMER_H
#include "rtp_spliter.h"
#include <stdio.h>

class RecordStreamer
{
 public:
   RecordStreamer();
   ~RecordStreamer();

   bool OpenRecordFile(const char* fname);
   //for nvr
   bool OpenRecordFile(const char* fname, uint32_t start_time, uint32_t stop_time);

   //get record file head
   int GetFileHead(const char** data, int* len);

   //move the i frame near the time
   bool MoveTo(const char* data);
   //@start_time  offset of seconds to begining
   bool MoveTo(int start_time);
   bool MoveTo(int hour, int min, int sec);

   //0 success
   //for performance reason, get BUFSIZ bytes not one frame 
   int GetNextFrame(const char** frame, int* len);

   void Close();

 private:
   FILE* m_file;
   RTPSpliter m_spliter;
   uint32_t   m_filesize;
   //seconds base at 00:00:00
   int   m_start_time;
   //seconds base at 00:00:00
   int   m_end_time;
   char* m_read_buf;
   // for nvr
   uint32_t m_file_offset;
   char m_file_path[128];
};


#endif /* end of include guard: RECORDSTREAMER_H */
