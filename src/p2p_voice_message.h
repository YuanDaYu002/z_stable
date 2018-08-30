#ifndef _P2P_VOICE_MESSAGE_H_
#define _P2P_VOICE_MESSAGE_H_

#define P2P_MAX_VOICE_MESSAGE 	5

#include <string>

struct p2p_voice_message_info_t
{
	std::string mid;
	std::string voice_message_path;
	std::string url;
	int   use;
};

class VoiceMessageKeeper
{
public:
	VoiceMessageKeeper();
	~VoiceMessageKeeper();
	
	int Parse(const char* json);
	int Load();
	int Save();
    void Clear();
	const char* GetFilePath(const char* mid);
	/* 获取当前使用留言的下标 */
	int GetUseIndex();
private:
	bool HasMid(const char* mid);
    void FromLoad(const char* mid, const char* path, int use);
private:
	p2p_voice_message_info_t m_voice_message[P2P_MAX_VOICE_MESSAGE];
};

#endif
