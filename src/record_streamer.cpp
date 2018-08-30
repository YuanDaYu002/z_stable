#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "plog.h"

#include "device_operation.h"

#include "zmd_msg.h"

#include "record_streamer.h"

#define VIDEO_FRAME_HEAD_SIZE sizeof(VideoFrameHeader)

const unsigned int i_frame_head = 0x63643030; /*00dc*/
const unsigned int p_frame_head = 0x63643130; /*01dc*/
const unsigned int audio_head   = 0x62773130; /*01wb*/
typedef struct _frame_head
{
    unsigned int  flag;
    unsigned int  frame_len;
}frame_head_t;

RecordStreamer::RecordStreamer():m_file(NULL), m_filesize(0),
    m_start_time(0), m_end_time(0), m_read_buf((char*)malloc(BUFSIZ))
{

}

RecordStreamer::~RecordStreamer()
{
    plog("\n");
    if(m_read_buf)
        free(m_read_buf);
    Close();
}


static unsigned int rs_strtime2sec(const char* tm)
{
    //231046  hour:23, min:10, sec:46
    //return 23*3600 + 10*60 + 46
    int hour=0;
    int min=0;
    int sec=0;

    sscanf(tm, "%02d%02d%02d", &hour, &min, &sec);
    return hour*3600+min*60+sec;
}
bool RecordStreamer::OpenRecordFile(const char* fname)
{
    char name[70];
    char head[70];
    char date[20];
    char time1[20];
    char time2[20];

    plog("\n");
    //recfile_-131023-231046-000000-51010100.264
    //hdd00/p01/record/2013-10-23/recfile_-131023-211044-221044-51010100.264
    const char* pos = strrchr(fname, '/');
    if(!pos) return false;

    strcpy(name, pos+1);

    sscanf(name, "%[^-]-%[^-]-%[^-]-%[^-]-", head, date, time1, time2);

    m_start_time = rs_strtime2sec(time1);
    m_end_time   = rs_strtime2sec(time2);
    //for 00:00:00
    if(m_end_time == 0)
        m_end_time = 24*3600;
    plog("get start_time(%d) end_time(%d) from %s\n", m_start_time, m_end_time, fname);

    m_file = p2p_fopen(fname, "rb");
    if(m_file)
    {
        struct stat buf;
        fstat(fileno(m_file), &buf);
        m_filesize = buf.st_size;
        plog("filesize=%d\n", m_filesize);
    }
    return m_file != NULL;
}
extern int ChangeClientOffset(char* FileName, unsigned int Startime, unsigned int Endtime, unsigned int* FilePos, unsigned int* FileLen);
bool  RecordStreamer::OpenRecordFile(const char* fname, uint32_t start_time, uint32_t stop_time)
{
    memset(m_file_path, 0, sizeof(m_file_path));
    strncpy(m_file_path, fname, sizeof(m_file_path));
    m_file = p2p_fopen(fname, "rb");

    if(m_file)
    {
        struct tm date_tm;
        time_t date_sec;
        /* change seconds from absolute to relative value */
        gmtime_r((time_t*)&start_time, &date_tm);
        date_tm.tm_hour = 0;
        date_tm.tm_min = 0;
        date_tm.tm_sec = 0;
        date_sec = mktime(&date_tm);
#ifdef PLAYBACK
        if(ChangeClientOffset((char*)fname, start_time-date_sec, stop_time-date_sec, &m_file_offset, &m_filesize))
        {
            plog("change offset failed!\n");
            return false;
        }
#else
        plog("\n\n error : this build is not support remote playback!!!\n\n");
#endif

        m_start_time = start_time-date_sec;
        plog("filesize=%u\n", m_filesize);
        plog("fileoffset=%u\n", m_file_offset);
    }
    return m_file != NULL;
}
int RecordStreamer::GetFileHead(const char** data, int* len)
{
    plog("\n");
    char buf[512];

    P2P_ASSERT(m_file != NULL, -1);

    if(1 != fread(buf, sizeof(buf), 1, m_file))
        return -1;

    m_spliter.SplitFrame(CMD_TYPE_IPC_CLIENT, buf, sizeof(buf), 0, 0);
    *data = (char*)m_spliter.GetSendBuffer();
    *len  = m_spliter.GetSendBufferSize();

    return 0;
}
bool RecordStreamer::MoveTo(const char* data)
{
    char head[32];
    int  hour=0;
    int  min=0;
    int  sec=0;

    sscanf(data, "%[^ ] %d:%d:%d", head, &hour, &min, &sec);

    plog("moveto:%d %d %d\n", hour, min, sec);
    return MoveTo(hour, min, sec);
}
bool RecordStreamer::MoveTo(int start_time)
{
    plog("moveto: %d\n", start_time);

    int hour = (m_start_time+start_time)/3600;
    int min  = ((m_start_time+start_time)%3600)/60;
    int sec  = ((m_start_time+start_time)%3600)%60;
    return MoveTo(hour, min, sec);
}
extern int getRecordOffset(const char* path, unsigned int start_time, unsigned int *pOffset);

bool RecordStreamer::MoveTo(int hour, int min, int sec)
{
    plog("%02d:%02d:%02d\n", hour, min, sec);
    P2P_ASSERT(m_file != NULL, false);

    int offset_time = hour*3600 + min*60 + sec;
    uint32_t offset = 0;

    plog("fpath:%s\n", m_file_path);
#ifdef PLAYBACK
    if(0 == getRecordOffset(m_file_path, offset_time, &offset))
    {
        fseek(m_file, offset, SEEK_SET);
        plog("offset[%u]\n", offset);
        return true;
    }
#else
    plog("\n\n error : this build is not support remote playback!!!\n\n");
#endif
    //seek to file head
    fseek(m_file, 512, SEEK_SET);
    return true;
}
int RecordStreamer::GetNextFrame(const char** frame, int* len)
{
    P2P_ASSERT(m_file != NULL, -1);
    P2P_ASSERT(m_read_buf != NULL, -1);

    if(!feof(m_file))
    {
        *len = fread(m_read_buf, 1, BUFSIZ, m_file);
        if(*len <= 0)
            return -1;

        m_spliter.SplitFrame(CMD_TYPE_IPC_CLIENT, m_read_buf, *len, 0, 0);
        *frame = m_spliter.GetSendBuffer();
        *len   = m_spliter.GetSendBufferSize();
        return 0;
    }
    else
        return -1;
    return 0;
}
void RecordStreamer::Close()
{
    if(m_file)
    {
        fclose(m_file);
        m_file = NULL;
    }
}

#if 0
int main(int argc, const char *argv[])
{
    RecordStreamer *stream = new RecordStreamer;
    const char* data = NULL;
    int len = 0;

    stream->OpenRecordFile("/hdd00/p01/record/2013-10-23/recfile_-131023-211044-221044-51010100.264");
    stream->GetFileHead(&data, &len);
    if(stream->MoveTo(21,51, 22, &data, &len))
    {
        printf("%s :%d\n", "bingo", len);
    }
    return 0;
}
#endif
