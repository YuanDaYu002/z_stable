#ifndef INTERFACEDEF_H

#define INTERFACEDEF_H

#pragma pack( 1 )


typedef enum 
{
    CMD_STOP = 0,
    CMD_LEFT,
    CMD_RIGHT,
    CMD_UP,
    CMD_DOWN,
     
    CMD_CALL_CRIUSE = 0x12, // 呼叫巡航
     
    CMD_AUTOSCAN = 0x13,
     
    CMD_CALLPRESET = 0x15,
    CMD_CALL_KINDSCAN = 0x16,  //呼叫花样扫描
     
    CMD_FOCUSFAR = 0x23,
    CMD_FOCUSNAER = 0x24,
    CMD_IRISOPEN = 0x25,
    CMD_IRISCLOSE = 0x26,
    CMD_ZOOMTELE = 0x27,
    CMD_ZOOMWIDE  = 0x28,
     
    /*CMD_SET_CRIUSE_P = 0x32, //设置巡航点*/
    CMD_SETPRESET = 0x35,
    CMD_CRIUSE = 0x36,
    CMD_CLRPRESET = 0x37,
    CMD_STOPSCAN = 0x38,
     
    CMD_SET_DWELLTIME=0x39,
     
    CMD_KINDSCAN_START= 0x3A,
     
    CMD_KINDSCAN_END= 0x3B,
     
    CMD_CLRCRIUES_LINE= 0x3C,
     
    CMD_CLR_SCAN_LINE = 0x3D,
    CMD_CLR_KINDSCAN= 0x3E,
     
     
    CMD_AUTOSCANSPEED = 0x3F,
     
    CMD_SET_PRESET = 0x40,
     
    CMD_CAL_PRESET = 0x41,
     
    CMD_GET_PRESET = 0x42,
     
    CMD_SET_RESET = 0x43, /*恢复出厂值*/
     
    CMD_V_SCAN = 0x44,
    CMD_DEL_PRESET = 0x45,
}PTZ_CMD_E;
typedef struct
{
    int						head;
    int						length;		//数据长度,去除head
    unsigned char			type;
    unsigned char			channel;
    unsigned short			commd;
}Cmd_Header ;

typedef struct
{
    Cmd_Header head;
    PTZ_CMD_E	cmd;
    unsigned short para0;
    unsigned short para1;
}STRUCT_SET_PTZ_REQUEST ;

#pragma pack()

#endif /* end of include guard: INTERFACEDEF_H */
