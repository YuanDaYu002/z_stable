#ifndef P2P_ACCESS_CLIENT_H

#define P2P_ACCESS_CLIENT_H

/*
 * interfaces for access module
 * */

#include "p2p_def.h"
#include "access.h"

int p2p_access_client_init();

int p2p_access_client_start(p2p_base_t *base);

int p2p_access_client_if_stop();

int p2p_access_client_if_connected();

int p2p_access_client_get_transinfo(int chlnum, int media_type, int usage);

int p2p_access_client_get_server_timestamp();

int p2p_access_client_set_callback( AccessMsgHandler, AccessNetworkCallback);

int p2p_access_client_stop();

#endif /* end of include guard: P2P_ACCESS_CLIENT_H */
