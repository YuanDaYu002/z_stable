#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ac_msg_handler.h"
#include "plog.h"
#include "device_operation.h"
#include "helpfunction.h"
#include "tcp_socket.h"
#include "zmdtimer.h"
#include "ac_msg.h"
#include "p2p_streamer.h"
#include "zmd_cloud.h"

extern int start_streamer(const char* mode, const char* fpath);


/* 在此增加消息处理函数*/
#define HANDLER_LIST(OP)\
    OP(DEV_SRV_RECV_TRANS_SYN, ac_handle_trans)\
    OP(DEV_SRV_RECV_UPDATE_SYN, ac_handle_upgrade)\
    OP(DEV_SRV_FORCE_I_FRAME_SYN, ac_handle_force_iframe)\
    OP(DEV_SRV_SYNC_SYN, ac_handle_sync)\
    OP(CLI_DEV_RESET_SYN, ac_handle_reset)\
    OP(DEV_SRV_REFRESH_SYN, ac_handle_reflash_cover)\
    OP(CLI_DEV_PLAY_VOICE_SYN, ac_handle_play_voice)\
	OP(DEV_SRV_ANSWER_SYN, ac_handle_answer)\
    OP(CLI_DEV_REFUSE_SYN, ac_handle_refuse_answer)\
    OP(CLI_DEV_HANGUP_SYN, ac_handle_hang_up)\
	OP(CLI_DEV_SDCARD_OPERATION_SYN, ac_handle_sdcard_operation)\
	OP(CLI_DEV_SDCARD_STATUS_SYN, ac_handle_get_sdcard_status)\
    OP(CLI_DEV_PTZ_SYN, ac_handle_ptz)\
    OP(DEV_SRV_GET_TRANSINFO_ACK, ac_handle_get_trans)\
    OP(DEV_SRV_APMODE_SYN, ac_handle_ap_mode)\
    OP(CLI_DEV_TEST_BUZZER_SYN, ac_handle_test_buzzer)\
    OP(CLI_DEV_AREA_DETECTION_SYN, ac_handle_set_area_detection)\
    OP(CLI_DEV_GET_AREA_DETECTION_SYN, ac_handle_get_area_detection)\
    OP(CLI_DEV_SPRINKLER_OP_SYN, ac_handle_watering_ctrl)\
    OP(CLI_DEV_CURTAIN_STATUS_SYN, ac_handle_get_curtain_stat)\
    OP(CLI_DEV_CURTAIN_OP_SYN, ac_handle_curtain_ctrl)\
    OP(CLI_DEV_ADD_PRESET_SYN, ac_handle_add_preset)\
    OP(CLI_DEV_MODIFY_PRESET_SYN, ac_handle_modify_preset)\
    OP(CLI_DEV_LOCK_OP_SYN, ac_handle_lock_ctrl)\
    OP(DEV_SRV_COM_NOTIFY_SYN, ac_handle_common_notify)
	
/*   handler helper */
#define DECLARE_HANDLER_ARRAY(M,F)\
	{M, F},
#define DECLARE_FUNCS(M,F)\
	char* F(JsonBuilder&, JsonParser&);

/* declare handler functions */
HANDLER_LIST(DECLARE_FUNCS)

struct ac_msg_handler_t
{
    int cmd;
    char* (*handler)(JsonBuilder& , JsonParser&);
};

/* define handler list */
static ac_msg_handler_t  handle_fun[] =
{
	HANDLER_LIST(DECLARE_HANDLER_ARRAY)
	{NULL, NULL}
};

#undef HANDLER_LIST
#undef DECLARE_FUNCS
#undef DECLARE_HANDLER_ARRAY


void ac_build_simple_response(JsonBuilder& enc, JsonParser& dec, int cmd,
        int result_code, const char* result_reason) 
{
    enc.AddInt( "type", AC_MSG_DATA);
    enc.AddInt( "length", 0);
    enc.AddInt( "message_id", dec.GetIntValue("message_id"));
    enc.AddInt( "cmd", cmd);
    enc.AddString("to_id", dec.GetStringConst("from_id"));
    enc.AddString("from_id", dec.GetStringConst("to_id"));
    enc.AddInt( "result_code", result_code);
    enc.AddString("result_reason", result_reason);
}

char* ac_msg_handler(const char* msg, int msg_size) 
{
    JsonParser dec(msg);
    JsonBuilder enc;

    int cmd_type = dec.GetIntValue("cmd");
    for (int i = 0; handle_fun[i].cmd != 0; i++) 
    {
        if (cmd_type == handle_fun[i].cmd)
        {
            char* result = handle_fun[i].handler(enc, dec);
            if(result) 
                return strdup(result);
            else
                return NULL;
        }
    }
    plog("recv unknow message %d\n", cmd_type);
    ac_build_simple_response(enc, dec, 0, 1, "unknow message");
    return strdup(enc.GetJson());
}

char* ac_handle_trans(JsonBuilder& enc, JsonParser& dec) 
{
    plogfn();
    const char* id = dec.GetStringConst("id");
    const char* ip = dec.GetStringConst("trans_ip");
    int port = dec.GetIntValue("trans_port");
    int chlnum = dec.GetIntValue("channel_id");
    int streamtype = dec.GetIntValue("media_type");

    if(streamtype != MEDIA_TYPE_PLAYBACK && !p2p_is_channel_valid(chlnum, streamtype))
    {
        if(p2p_is_channel_valid(chlnum, STREAMTYPE_VGA))
            streamtype = STREAMTYPE_VGA;
        else if(p2p_is_channel_valid(chlnum, STREAMTYPE_QVGA))
            streamtype = STREAMTYPE_QVGA;
        else if(p2p_is_channel_valid(chlnum, STREAMTYPE_720P))
            streamtype = STREAMTYPE_720P;
        else
        {
            plog("stream is invalid!!!\n");
            goto ERR;
        }
    }

    if(streamtype != MEDIA_TYPE_PLAYBACK && !p2p_is_device_on() )
    {
        plog("%s\n", "device is off!!!");
        goto ERR;
    }

    p2p_on_video_request(streamtype, chlnum);

    plog("start tcp handler with:\nid:%s\nip:%s\nport:%d\nstreamtype:%d\nchlnum%d\n",
            id, ip, port, streamtype, chlnum);

    p2p_start_trans_streamer(id, ip, port, chlnum, streamtype, p2p_get_deviceid());


    ac_build_simple_response(enc, dec, DEV_SRV_RECV_TRANS_ACK, 0, "OK");

    return enc.GetJson();
ERR:
    ac_build_simple_response(enc, dec, DEV_SRV_RECV_TRANS_ACK, 2, "channel invalid or device is off!");

    return enc.GetJson();
}

/* update functions */
extern int p2p_do_upgrade(const char *url);
volatile int p2p_update_flag = 0;
void update_thread(void* param)
{
    char* url = (char*)param;

    //for thread safe
    if(p2p_update_flag == 0)
    {
        p2p_update_flag = 1;
        //p2p_update_system(url);
        p2p_do_upgrade(url);
        p2p_update_flag = 0;
    }
    free(url);
}

char* ac_handle_upgrade(JsonBuilder& enc, JsonParser& dec) 
{
    char *url = (char*)malloc(512);

    dec.GetStringValue(url, 512, "url");

    p2p_long_task_p(update_thread, url);
    ac_build_simple_response(enc, dec, DEV_SRV_RECV_UPDATE_ACK, 0, "OK");

    plog("return success\n");
    return enc.GetJson();
}

char* ac_handle_force_iframe(JsonBuilder& enc, JsonParser& dec) 
{
    plogfn();

    p2p_force_i_frame(dec.GetIntValue("channel_num"), dec.GetIntValue("media_type"));
    ac_build_simple_response(enc, dec, DEV_SRV_FORCE_I_FRAME_ACK, 0, "OK");

    return enc.GetJson();
}

static void p2p_reset_timer(void* para) 
{
    int oper_type = (int)para;

    if (oper_type == 1)
        p2p_reset_device();
    else if (oper_type == 2)
        p2p_reboot_device();
}

char* ac_handle_reset(JsonBuilder& enc, JsonParser& dec) 
{
    plogfn();
    ac_build_simple_response(enc, dec, CLI_DEV_RESET_ACK, 0, "OK");

    ZmdCreateShortTimer(3000, p2p_reset_timer, (void*)dec.GetIntValue("op_type"));

    return enc.GetJson();
}

extern void sync_paramter(int);

char* ac_handle_sync(JsonBuilder& enc, JsonParser& dec)
{
    ac_build_simple_response(enc, dec, DEV_SRV_SYNC_ACK, 0, "OK");

    sync_paramter(0);

    return enc.GetJson();
}

char* ac_handle_reflash_cover(JsonBuilder& enc, JsonParser& dec)
{
    ac_build_simple_response(enc, dec, DEV_SRV_REFRESH_ACK, 0, "OK");

    p2p_on_refresh_cover();

    return enc.GetJson();
}

char* ac_handle_play_voice(JsonBuilder& enc, JsonParser& dec)
{
    ac_build_simple_response(enc, dec, CLI_DEV_PLAY_VOICE_ACK, 0, "OK");

	p2p_play_voice(dec.GetStringConst("voice_message"));

    return enc.GetJson();
}

char* ac_handle_answer(JsonBuilder& enc, JsonParser& dec)
{
    ac_build_simple_response(enc, dec, DEV_SRV_ANSWER_ACK, 0, "OK");

    p2p_handle_answer_call();
    return enc.GetJson();
}

char* ac_handle_refuse_answer(JsonBuilder& enc, JsonParser& dec)
{
    ac_build_simple_response(enc, dec, CLI_DEV_REFUSE_ACK, 0, "OK");
	p2p_on_refuse_answer();
    return enc.GetJson();
}

char* ac_handle_hang_up(JsonBuilder& enc, JsonParser& dec)
{
    ac_build_simple_response(enc, dec, CLI_DEV_HANGUP_ACK, 0, "OK");
	p2p_on_hang_up();
    return enc.GetJson();
}

char* ac_handle_sdcard_operation(JsonBuilder& enc, JsonParser& dec)
{
	int oper_type = -1;

	plog("\n");
    dec.GetIntValue(oper_type, "sd_type");
    if(oper_type != -1)
    {
        ac_build_simple_response(enc, dec, CLI_DEV_SDCARD_OPERATION_ACK, 0, "OK");
		if(0 == p2p_format_sd_card())
			return enc.GetJson();
    }

    ac_build_simple_response(enc, dec, CLI_DEV_SDCARD_OPERATION_ACK, 1, "FAILED");
    return enc.GetJson();
}

char* ac_handle_get_sdcard_status(JsonBuilder& enc, JsonParser& dec)
{
    ac_build_simple_response(enc, dec, CLI_DEV_SDCARD_STATUS_ACK, 0, "OK");


	SD_CARD_STAT st;

	plog("\n");
	p2p_get_sd_card_status(&st);

    enc.AddInt("sd_card", st.status);
    enc.AddInt("free", st.free_size);
    enc.AddInt("used", st.used_size);

    return enc.GetJson();
}

char* ac_handle_ptz(JsonBuilder& enc, JsonParser& dec)
{
    ac_build_simple_response(enc, dec, CLI_DEV_PTZ_ACK, 0, "OK");

    p2p_ptz_operat(dec.GetIntValue("channel"),
            dec.GetIntValue("ptz_cmd"),
            dec.GetIntValue("para0"),
            dec.GetIntValue("para1"));
    enc.AddString("uid", dec.GetStringConst("uid"));
    return enc.GetJson();
}
  
char* ac_handle_get_trans(JsonBuilder& enc, JsonParser& dec)
{
    plogfn();
    int msg_id = dec.GetIntValue("message_id");
    const char* id = dec.GetStringConst("register_code");
    const char* ip = dec.GetStringConst("trans_ip");
    int port = dec.GetIntValue("trans_port");
    int ret_code = dec.GetIntValue("result_code");
    //ZMDBELL03081557_0_1&3991293055

    int chlnum = -1;
    int streamtype = -1;
    char tmp[32];

    if(ret_code == 0 && id && ip)
    {
        sscanf(id, "%[^_]_%d_%d", tmp, &chlnum, &streamtype);

        plog("start tcp handler with:\nid:%s\nip:%s\nport:%d\nstreamtype:%d\nchlnum:%d\n",
                id, ip, port, streamtype, chlnum);
        if(msg_id == 1022111) 
            p2p_start_trans_streamer(id, ip, port, chlnum, streamtype, p2p_get_deviceid());
        else if( msg_id == 2022111 || msg_id == 3022111 || msg_id == 4022111)
        {
            s_transit_server_info ts;
            strcpy(ts.server_ip, ip);
            strcpy(ts.identity, id);
            ts.server_port = port;
#ifndef  _BUILD_FOR_NVR_
            cld_set_trans_info(&ts);
#endif
        }
    }
    return NULL;
}

char* ac_handle_ap_mode(JsonBuilder& enc, JsonParser& dec)
{
    int mode = dec.GetIntValue("mode");
    int device_count = dec.GetIntValue("device_count");
    const char* user_id = dec.GetStringConst("user_id");

    p2p_set_ap_mode(mode, device_count, user_id);
    ac_build_simple_response(enc, dec, DEV_SRV_APMODE_ACK, 0, "OK");
    return enc.GetJson();
}

char* ac_handle_test_buzzer(JsonBuilder& enc, JsonParser& dec)
{
    int oper = dec.GetIntValue("op_type");
    p2p_set_buzzer(oper);
    ac_build_simple_response(enc, dec, CLI_DEV_TEST_BUZZER_ACK, 0, "OK");
    return enc.GetJson();
}

char* ac_handle_set_area_detection(JsonBuilder& enc, JsonParser& dec)
{
    p2p_md_region_t reg;

    memset(&reg, 0, sizeof(reg));

    if(dec.GetIntValue("data_type") != 1)
    {
        dec.GetFloatValue(reg.x, "x");
        dec.GetFloatValue(reg.y, "y");
        dec.GetFloatValue(reg.width, "width");
        dec.GetFloatValue(reg.height, "height");
        reg.data_type = 0;
    }
    else
    {
        for (int i = 0; i < 8; ++i)
        {
            char name[32]={0};
            sprintf(name, "point%d", i);
            const char* value = dec.GetStringConst(name);  
            if(value)
            {
                JsonParser js(value);
                js.GetFloatValue(reg.octagon[i].x, "x");
                js.GetFloatValue(reg.octagon[i].y, "y");
            } 
        } 
        reg.data_type = 1;
    }

#ifdef _BUILD_FOR_NVR_
    dec.GetIntValue(reg.channel, "channel");
    dec.GetStringValue(reg.physical_id, 16, "physical_id");
#endif

    if(0 == p2p_md_region_ctrl(1, &reg)) 
        ac_build_simple_response(enc, dec, CLI_DEV_AREA_DETECTION_ACK, 0, "OK");
    else
        ac_build_simple_response(enc, dec, CLI_DEV_AREA_DETECTION_ACK, 1, "callback function no set!");

    return enc.GetJson();
}

char* ac_handle_get_area_detection(JsonBuilder& enc, JsonParser& dec)
{
    p2p_md_region_t reg;

    memset(&reg, 0, sizeof(reg));

#ifdef _BUILD_FOR_NVR_
    dec.GetIntValue(reg.channel, "channel");
    dec.GetStringValue(reg.physical_id, 16, "physical_id");
#endif

    if(0 == p2p_md_region_ctrl(2, &reg)) 
        ac_build_simple_response(enc, dec, CLI_DEV_GET_AREA_DETECTION_ACK, 0, "OK");
    else
        ac_build_simple_response(enc, dec, CLI_DEV_GET_AREA_DETECTION_ACK, 1, "callback function no set!");

    if(reg.data_type != 1)
    {
        enc.AddString("x", p2p_ftoa(reg.x));
        enc.AddString("y", p2p_ftoa(reg.y));
        enc.AddString("width", p2p_ftoa(reg.width));
        enc.AddString("height", p2p_ftoa(reg.height));
        enc.AddInt("data_type", 0);
    }
    else
    {
        for (int i = 0; i < 8; ++i)
        {
            JsonBuilder js;
            js.AddString("x", p2p_ftoa(reg.octagon[i].x));
            js.AddString("y", p2p_ftoa(reg.octagon[i].y));
            char name[32]={0};
            sprintf(name, "point%d", i);
            enc.AddJson(name, js);   
        }

        enc.AddInt("data_type", 1);
    }

    return enc.GetJson();
}

char* ac_handle_watering_ctrl(JsonBuilder& enc, JsonParser& dec)
{
    p2p_watering_ctrl_t wc;

    memset(&wc, 0, sizeof(wc));
    dec.GetIntValue(wc.op_type, "op_type");
    dec.GetStringValue(wc.physical_id, 32, "physical_id");
    dec.GetIntValue(wc.hole_id, "hole_id");
    dec.GetIntValue(wc.time, "time");

    if(0 == p2p_watering_ctrl(&wc)) 
        ac_build_simple_response(enc, dec, CLI_DEV_SPRINKLER_OP_ACK, 0, "OK");
    else
        ac_build_simple_response(enc, dec, CLI_DEV_SPRINKLER_OP_ACK, 1, "callback function no set!");

    return enc.GetJson();
}

char* ac_handle_get_curtain_stat(JsonBuilder& enc, JsonParser& dec)
{
    int curtain_status = 0;
    int screen_status = 0;

    if(0 == p2p_get_curtain_stat(&curtain_status, &screen_status)) 
        ac_build_simple_response(enc, dec, CLI_DEV_CURTAIN_STATUS_ACK, 0, "OK");
    else
        ac_build_simple_response(enc, dec, CLI_DEV_CURTAIN_STATUS_ACK, 1, "callback function no set!");
    enc.AddInt("curtain_status", curtain_status);
    enc.AddInt("screen_status", screen_status);

    return enc.GetJson();
}

char* ac_handle_curtain_ctrl(JsonBuilder& enc, JsonParser& dec)
{
    if(0 == p2p_curtain_ctrl(dec.GetIntValue("op_type"), dec.GetIntValue("is_curtain")))
        ac_build_simple_response(enc, dec, CLI_DEV_CURTAIN_OP_ACK, 0, "OK");
    else
        ac_build_simple_response(enc, dec, CLI_DEV_CURTAIN_OP_ACK, 1, "callback function no set!");

    return enc.GetJson();
}

char* ac_handle_add_preset(JsonBuilder& enc, JsonParser& dec)
{
    const char* physical_id = dec.GetStringConst("physical_id");
    const char* preset_name = dec.GetStringConst("preset_name");
    char* image_url = NULL;

    if(0 == p2p_preset_ctrl(0, physical_id, preset_name, NULL, &image_url ))
    {
        ac_build_simple_response(enc, dec, CLI_DEV_ADD_PRESET_ACK, 0, "OK");
		if(image_url)
		{
        	enc.AddString("image_url", image_url);
        	free(image_url);
		}
    }
    else
        ac_build_simple_response(enc, dec, CLI_DEV_ADD_PRESET_ACK, 1, "add preset failed");

    return enc.GetJson();
}

char* ac_handle_modify_preset(JsonBuilder& enc, JsonParser& dec)
{
    const char* physical_id = dec.GetStringConst("physical_id");
    const char* preset_name = dec.GetStringConst("preset_name");
    const char* name = dec.GetStringConst("name");
    char* image_url = NULL;

    if(0 == p2p_preset_ctrl(1, physical_id, preset_name, name, &image_url ))
    {
        ac_build_simple_response(enc, dec, CLI_DEV_MODIFY_PRESET_ACK, 0, "OK");
        if(image_url)
		{
        	enc.AddString("image_url", image_url);
        	free(image_url);
		}
    }
    else
        ac_build_simple_response(enc, dec, CLI_DEV_MODIFY_PRESET_ACK, 1, "modify preset failed");

    return enc.GetJson();
}

char* ac_handle_lock_ctrl(JsonBuilder& enc, JsonParser& dec)
{
    const char* physical_id = dec.GetStringConst("physical_id");
    int op_type = dec.GetIntValue("op_type");

    int ret = -1;

    if(physical_id)
        ret=p2p_lock_ctrl(op_type, physical_id);
    if(ret == 0)
        ac_build_simple_response(enc, dec, CLI_DEV_LOCK_OP_ACK, 0, "OK");
    else
        ac_build_simple_response(enc, dec, CLI_DEV_LOCK_OP_ACK, 1, "unlock error");

    return enc.GetJson();
}

char* ac_handle_common_notify(JsonBuilder& enc, JsonParser& dec)
{
    int msg_type = dec.GetIntValue("msg_type");
    const char* content = dec.GetStringConst("content");

    if(msg_type == 2 && content)
    {      
        JsonParser jp(content);

        const char* physical_id = jp.GetStringConst("physical_id");
        int op_type = jp.GetIntValue("op_type");

        int ret = -1;

        if(physical_id)
            ret=p2p_lock_ctrl(op_type, physical_id);
        if(ret == 0)
            ac_build_simple_response(enc, dec, DEV_SRV_COM_NOTIFY_ACK, 0, "OK");
        else
            ac_build_simple_response(enc, dec, DEV_SRV_COM_NOTIFY_ACK, 1, "unlock error");  
    }
    else
    {
        ac_build_simple_response(enc, dec, DEV_SRV_COM_NOTIFY_ACK, 2, "unknow command");
    }

    return enc.GetJson();
}
