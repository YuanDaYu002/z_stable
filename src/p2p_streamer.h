#ifndef P2P_STREAM_H_KAKDI9JLDFA54ZMDOQOF_
#define P2P_STREAM_H_KAKDI9JLDFA54ZMDOQOF_


#define MEDIA_TYPE_PLAYBACK 6
#define MEDIA_TYPE_RTSP 7
#define MEDIA_TYPE_RTMP 8

void p2p_start_trans_streamer(const char* id, const char* trans_ip, int trans_port,
                                         int channel, int streamtype, const char* devid);

#endif /* end of P2P_STREAM_H_KAKDI9JLDFA54ZMDOQOF_*/
