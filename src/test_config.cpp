#include <algorithm>    // std::random_shuffle
#include <vector>       // std::vector
#include <ctime>        // std::time
#include <map>
#include "test_config.h"
#include "plog.h"
#include "device_operation.h"
#include "helpfunction.h"
#include "p2p_def.h"
#include "zmutex.h"

extern p2p_init_info_t g_p2p_envir;

std::string get_web_server()
{
    if(getenv("P2P_SERVER_ADDR"))
        return getenv("P2P_SERVER_ADDR");
    else
        return get_dev_access();
}

struct ServerState
{
    bool valid; // true ok, false invalid
    int last_change_time;
};

typedef std::map<std::string, ServerState> ServerStateMap;
typedef std::vector<std::string> ServerNameList;

static ServerStateMap g_server_state;

static ServerNameList g_dev_access_servers;
static ServerNameList g_devconn_servers;
static ServerNameList g_alerts_servers;
static ServerNameList g_file_servers;
static ServerNameList g_devmng_servers;
static ZMutex g_servers_lock;

static void random_sort(ServerNameList & v)
{
    //无rtc的设备每次启动调用time(NULL)的结果是一样的，这里使用设备ID来差异化
    static unsigned int hash_code = 0;

    if(hash_code == 0)
    {
        const char* devid = p2p_get_deviceid();
        hash_code = p2p_hash(devid, strlen(devid));
    }

    std::srand(hash_code);
    // using built-in random generator:
    std::random_shuffle ( v.begin(), v.end() );
}
static void save_servers(const char* servers, ServerNameList &out)
{
    const char* pos = servers;

    out.clear();

    do
    {
        char addr[256]="";
        sscanf(pos, "%[^,]", addr);

        if(strlen(addr))
        {
            out.push_back(addr);
            g_server_state[addr].valid = true;
            g_server_state[addr].last_change_time = p2p_get_uptime();
        }
        else
            break;
        pos = strchr(pos, ',');
        if(pos) 
            pos+=1;
        else
            break;

    }while(*pos);
    random_sort(out);
}
void set_dev_access_servers(const char* servers)
{
    if(servers) plog("%s\n", servers);
    //ZMutexLock l(&g_servers_lock);
    if(servers && strlen(servers))
    {
        save_servers(servers, g_dev_access_servers);
    }
}

std::string get_dev_access()
{
    static int robin = 0;

    if(robin == 0)
    {
        set_dev_access_servers(DEV_ACCESS);
    }

    ZMutexLock l(&g_servers_lock);
    int ssize = g_dev_access_servers.size();
    if(ssize)
    {
        int i = robin++%ssize;

        return g_dev_access_servers[i];
    }
    return "";
}

void set_devconn_servers(const char* servers)
{
    if(servers) plog("%s\n", servers);
    ZMutexLock l(&g_servers_lock);
    if(servers && strlen(servers))
    {
        save_servers(servers, g_devconn_servers);
    }
}

void get_devconn_server(char *ip, int* port)
{
    ZMutexLock l(&g_servers_lock);
    static int robin = 0;
    int ssize = g_devconn_servers.size();
    if(ssize)
    {
        //int i = p2p_get_rand(0, ssize) % ssize;
        int i = robin++%ssize;

        const std::string &addr = g_devconn_servers[i];
        sscanf(addr.c_str(), "%[^:]:%d", ip, port);
    }
}

void set_alerts_servers(const char* servers)
{
    if(servers) plog("%s\n", servers);
    ZMutexLock l(&g_servers_lock);

    if(servers && strlen(servers))
    {
        save_servers(servers, g_alerts_servers);
    }
}
void mark_invalid_server(const char* server)
{
    if(server && strlen(server))
    {
        ZMutexLock l(&g_servers_lock);
        plog("[%s] invalid\n", server);
        g_server_state[server].valid = false;
        g_server_state[server].last_change_time = p2p_get_uptime();
    }
}

static std::string get_valid_server(int &robin, ServerNameList &servers)
{
    int ssize = servers.size();
    int try_count = 0;

    int now = p2p_get_uptime();
    for(;;)
    {
        int i = robin++%ssize;
        const std::string &addr = servers[i];

        plog("server[%s]-valid[%d]-change_time[%d]\n", addr.c_str(), g_server_state[addr].valid, g_server_state[addr].last_change_time);
        if(g_server_state[addr].valid == false && now - g_server_state[addr].last_change_time > 30*60)
        {
            g_server_state[addr].valid = true;
            g_server_state[addr].last_change_time = now;
            plog("return server[%s] last_change_time[%d]\n", addr.c_str(), g_server_state[addr].last_change_time);
            return addr;
        }
        else if(g_server_state[addr].valid == true)
        {
            plog("return server[%s] last_change_time[%d]\n", addr.c_str(), g_server_state[addr].last_change_time);
            return addr;
        }
        if(try_count++ > ssize)
            break;
    }
    plog("all servers gone, mark all servers valid\n");
    for(int i=0; i<ssize; i++)
    {
        g_server_state[servers[i]].valid = true;
        g_server_state[servers[i]].last_change_time = now;
    }
    return servers[p2p_get_rand(0, ssize)%ssize];
}

std::string get_alerts_server()
{
    ZMutexLock l(&g_servers_lock);
    static int robin = 0;
    return get_valid_server(robin, g_alerts_servers);
}

void set_file_servers(const char* servers)
{
    if(servers) plog("%s\n", servers);
    ZMutexLock l(&g_servers_lock);
    if(servers && strlen(servers))
    {
        save_servers(servers, g_file_servers);
    }
}
std::string get_fileserver()
{
    ZMutexLock l(&g_servers_lock);
    static int robin = 0;
    return get_valid_server(robin, g_file_servers);
}

void set_devmng_servers(const char* servers)
{
    if(servers) plog("%s\n", servers);
    ZMutexLock l(&g_servers_lock);
    if(servers && strlen(servers))
    {
        save_servers(servers, g_devmng_servers);
    }
}
std::string get_devmng_server()
{
    ZMutexLock l(&g_servers_lock);
    static int robin = 0;
    return get_valid_server(robin, g_devmng_servers);
}

