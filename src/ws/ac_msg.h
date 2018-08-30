#ifndef AC_MSG_H
#define AC_MSG_H

typedef enum {
    AC_MSG_SHAKE = 0x01,
    AC_MSG_HEART_BEAT = 0x02,
    AC_MSG_DATA = 0x03,
    AC_MSG_REDIRECT = 0x04
} ac_msg_t;

typedef struct _ac_header1 {
    int  type:16;
    int  length:16;
    char body[0];
} ac_header1;

typedef struct _ac_header2 {
    int  message_id;
    int  cmd;
    char from_id[32];
    char to_id[32];
    char body[0];
} ac_header2;

typedef struct _ac_login_body {
    char cmd_version[12];
    int  data_type;
    char token_id[32];
    int  client_type;
    char client_version[12];
    char client_id[32];
} ac_login_body;
//proto types
typedef enum {
    PE_JSON,
    PE_BINARY,
    PE_PROTOBUF
} ProtoEncodeType;
typedef ProtoEncodeType ProtoDecodeType ;
typedef enum {
    VT_SHORT,
    VT_INT,
    VT_BOOL,
    VT_STRING
}WS_ValueType;
enum {
  CLI_DEV_PTZ_SYN = 0x01000009,
  CLI_DEV_PTZ_ACK = 0x01100009,
  CLI_DEV_SDCARD_OPERATION_SYN = 0x01000010,
  CLI_DEV_SDCARD_OPERATION_ACK = 0x01100010,
  CLI_DEV_SDCARD_STATUS_SYN   = 0x0100000f,
  CLI_DEV_SDCARD_STATUS_ACK   = 0x0110000f,
  CLI_DEV_MUTE_SYN	 = 0x01000011,
  CLI_DEV_MUTE_ACK	 = 0x01100011,
  CLI_DEV_RESET_SYN      = 0x01000012,
  CLI_DEV_RESET_ACK      = 0x01100012, 
  CLI_DEV_PLAY_VOICE_SYN = 0x01000013,
  CLI_DEV_PLAY_VOICE_ACK = 0x01100013,
  CLI_DEV_REFUSE_SYN = 0x01000014,
  CLI_DEV_REFUSE_ACK = 0x01100014,
  CLI_DEV_HANGUP_SYN = 0x01000015,
  CLI_DEV_HANGUP_ACK = 0x01100015,
  CLI_DEV_TEST_BUZZER_SYN = 0x01000016,
  CLI_DEV_TEST_BUZZER_ACK = 0x01100016,
  CLI_DEV_AREA_DETECTION_SYN = 0x01000017,
  CLI_DEV_AREA_DETECTION_ACK = 0x01100017,
  CLI_DEV_GET_AREA_DETECTION_SYN = 0x01000018,
  CLI_DEV_GET_AREA_DETECTION_ACK = 0x01100018,
  CLI_DEV_SPRINKLER_OP_SYN = 0x01000019,
  CLI_DEV_SPRINKLER_OP_ACK = 0x01100019,
  CLI_DEV_CURTAIN_STATUS_SYN = 0x01000020,
  CLI_DEV_CURTAIN_STATUS_ACK = 0x01100020,
  CLI_DEV_CURTAIN_OP_SYN = 0x01000021,
  CLI_DEV_CURTAIN_OP_ACK = 0x01100021,
  CLI_DEV_LOCK_OP_SYN = 0x01000022,
  CLI_DEV_LOCK_OP_ACK = 0x01100022,
  CLI_DEV_ADD_PRESET_SYN = 0x01000023,
  CLI_DEV_ADD_PRESET_ACK = 0x01100023,
  CLI_DEV_MODIFY_PRESET_SYN = 0x01000024,
  CLI_DEV_MODIFY_PRESET_ACK = 0x01100024,

  DEV_SRV_RECV_TRANS_SYN  =  0x03000001,
  DEV_SRV_RECV_TRANS_ACK  =  0x03100001,
  DEV_SRV_RECV_CONF_SYN   =  0x03000002,
  DEV_SRV_RECV_CONF_ACK   =  0x03100002,
  DEV_SRV_RECV_UPDATE_SYN =  0x03000003,
  DEV_SRV_RECV_UPDATE_ACK =  0x03100003,
  DEV_SRV_DOORBELL_SYN    =  0x03000005,
  DEV_SRV_DOORBELL_ACK    =  0x03100005,
  DEV_SRV_VOICEMSG_SYN    =  0x03000006,
  DEV_SRV_VOICEMSG_ACK    =  0x03100006,
  DEV_SRV_CHIME_CONF_SYN  =  0x03000007,
  DEV_SRV_CHIME_CONF_ACK  =  0x03100007,
  DEV_SRV_VOICEMSG_CONF_SYN = 0x03000008,
  DEV_SRV_VOICEMSG_CONF_ACK = 0x03100008,
  DEV_SRV_SENSITIVITY_CONF_SYN = 0x03000009,
  DEV_SRV_SENSITIVITY_CONF_ACK = 0x03100009,
  DEV_SRV_TIMETEMP_CONF_SYN	= 0x0300000A,
  DEV_SRV_TIMETEMP_CONF_ACK	= 0x0310000A,
  DEV_SRV_FORCE_I_FRAME_SYN   = 0x03000010,
  DEV_SRV_FORCE_I_FRAME_ACK   = 0x03100010,

  DEV_SRV_SYNC_SYN = 0x03000031,
  DEV_SRV_SYNC_ACK = 0x03100031,

  DEV_SRV_GET_TRANSINFO_SYN = 0x03000032,
  DEV_SRV_GET_TRANSINFO_ACK = 0x03100032,

  DEV_SRV_ANSWER_SYN = 0x03000033,
  DEV_SRV_ANSWER_ACK = 0x03100033,

  DEV_SRV_REFRESH_SYN = 0x03000034,
  DEV_SRV_REFRESH_ACK = 0x03100034,

  DEV_SRV_APMODE_SYN =  0x03000035,
  DEV_SRV_APMODE_ACK =  0x03100035,

  DEV_SRV_COM_NOTIFY_SYN = 0x03000036,
  DEV_SRV_COM_NOTIFY_ACK = 0x03100036,
}; 
//#define     PROTOTYPE   PE_BINARY
#define     PROTOTYPE   PE_JSON
#if 0
typedef struct _WS_Type_t {
    WS_ValueType type;
    int          len;
    char         name[32];
} WS_Type_t;

#define  WS_NEW_INT(name) WS_Type_t(VT_INT, 4, name) 
#define  WS_NEW_SHORT(name) WS_Type_t(VT_SHORT 2, name) 
#define  WS_NEW_STRING(name, len) WS_Type_t(VT_STRING len, name) 

#define  WS_NEW_CMD(CmdTypeName)\
struct CmdTypeName\
{\
    CmdTypeName()\
    {

#define WS_CMD_END\
    }\
    ~CmdTypeName() {}\
    std::vector<WS_Type_t> vars;\
}
#define  ADD_INT(paraname)         vars.push_back(WS_NEW_INT(paraname))
#define  ADD_SHORT(paraname)       vars.push_back(WS_NEW_SHORT(paraname))
#define  ADD_STRING(paraname, len) vars.push_back(WS_NEW_STRING(paraname, len))

WS_NEW_CMD(ws_cmd_ac_login_ack)
ADD_INT("code");
ADD_INT("heartbeat");
ADD_STRING("protos", 128);
WS_CMD_END;
#endif
#endif /* end of include guard: AC_MSG_H */
