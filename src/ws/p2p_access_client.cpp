
#include "p2p_access_client.h"
#include "ac_msg_handler.h"
#include "helpfunction.h"
#include "device_operation.h"
#include "test_config.h"

static AccessServerHandler *ac_ptr;

int p2p_access_client_init()
{
    ac_ptr = new AccessServerHandler;

    return 0;
}

extern char* ac_msg_handler(const char* msg, int msg_size);

int p2p_access_client_start(p2p_base_t *base)
{
    char devconn_ip[32]="";
    int  devconn_port = 0;
    get_devconn_server(devconn_ip, &devconn_port);
    return ac_ptr->StartHandler(devconn_ip, devconn_port, "V1.0", base->device_id,
            base->token, base->access_aes_key, base->aes_key_id);
}

int p2p_access_client_if_stop()
{
    return ac_ptr->IsStop();
}

int p2p_access_client_if_connected()
{
    return ac_ptr->IsAlive();
}

int p2p_access_client_get_transinfo(int chlnum, int media_type, int usage)
{
    return ac_ptr->GetTransInfo(chlnum, media_type, usage);
}

int p2p_access_client_get_server_timestamp()
{
    return ac_ptr->GetServerTimeStamp();
}

int p2p_access_client_set_callback(AccessMsgHandler hdl, AccessNetworkCallback cb)
{
    return ac_ptr->SetCallback(hdl, cb);
}

int p2p_access_client_stop()
{
    ac_ptr->StopHandler();
    return 0;
}
