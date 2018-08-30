#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>

//#define OTA_TEST 1
#ifdef AMBA
#include "amba.h"
#endif

#ifndef OTA_TEST
#include "plog.h"
#include "zsp_h.h"
#else
#define plog printf
#endif

#include "web.h"
#include "zmdtimer.h"
#include "zmd_task.h"
#include "device_operation.h"

#define PRI_STR(v) plog(#v" --> %s\n", v)
#define PRI_INT(v) plog(#v" --> %d\n", v)

#define HTTP_PREFIX "http://"
#define HTTP_CONTENT_LENGTH "Content-Length:"
#define UPDATEFILE  "/tmp/UpdateFile"

struct ota_base_t
{
    int fd;
    int continue_flag;
    int updatefile_body_size;
    int updatefile_recv_size;
    int download_offset;
    char* url;
    char updatefile_head[256];
    char failure_desc[256];
    struct addrinfo *res;
};
int ota_fd_writable(int fd, int msec)
{
    fd_set writefds;

    FD_ZERO(&writefds);

    FD_SET( fd, &writefds );
    int maxfd = fd + 1;

    timeval to;
    to.tv_sec = 0;
    to.tv_usec = msec*1000 ;

    int n = select(maxfd , NULL, &writefds, NULL, &to); 

    if( n > 0 ){
        if (FD_ISSET( fd, &writefds )){
            return 1;
        }
    }
    return 0;
}
int ota_fd_readable(int fd, int msec)
{
    fd_set readfds;

    FD_ZERO(&readfds);

    FD_SET( fd, &readfds );
    int maxfd = fd + 1;

    timeval to;
    to.tv_sec = 0;
    to.tv_usec = msec*1000 ;

    int n = select(maxfd ,&readfds, NULL,  NULL, &to); 

    if( n > 0 ){
        if (FD_ISSET( fd, &readfds )){
            return 1;
        }
    }
    return 0;
}
int ota_get_host_from_url(const char* url, char* server_name)
{
    const char* pos = strcasestr(url, HTTP_PREFIX);

    if(!pos) return -1;

    pos += strlen(HTTP_PREFIX);

    sscanf(pos, "%[^/]", server_name);
    return 0;
}

int ota_get_uri_from_url(const char* url, char* uri)
{
    const char* pos = strcasestr(url, HTTP_PREFIX);

    if(!pos) return -1;

    pos += strlen(HTTP_PREFIX);

    pos = strstr(pos, "/");

    //plog("%s\n", pos);

    strcpy(uri, pos);

    return 0;
}

int ota_get_server_name(ota_base_t *base)
{
    plog("url:[%s]\n", base->url);

    char tmp[128] = {0};
    char server_name[128] = {0};
    char server_port[32] = "80";

    struct addrinfo hints;

    ota_get_host_from_url(base->url, server_name);

    if(strstr(server_name, ":")>0)
    {
        strcpy(tmp, server_name);
        sscanf(tmp, "%[^:]:%s", server_name, server_port);
    }
    PRI_STR(server_name);
    PRI_STR(server_port);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = PF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(server_name, server_port,&hints,&base->res)) 
    {
        perror("getaddrinfo");
        return -1;
    }
    return 0;
}

int ota_connect_server(ota_base_t *base, int timeout)
{
    int err = 0, flags = 0, done = 0;

    fd_set wset;
    socklen_t optlen;
    struct timeval tv;
    base->fd = socket(AF_INET, SOCK_STREAM, 0);

    if(ota_get_server_name(base))
    {
        strcpy(base->failure_desc, "ota_get_server_name failed");
        return -1;
    }
    do
    {
        /* make socket non-blocking */
        flags = fcntl( base->fd, F_GETFL, 0 );
        if( fcntl(base->fd, F_SETFL, flags|O_NONBLOCK) != 0 )
            break;
        if(0 > connect(base->fd, base->res->ai_addr, base->res->ai_addrlen))
        {
            if( errno != EINPROGRESS )
                break;

            tv.tv_sec = 0;
            tv.tv_usec = 1000*timeout;

            FD_ZERO( &wset );
            FD_SET( base->fd, &wset );

            /* wait for connect() to finish */
            if( (err = select(base->fd + 1, NULL, &wset, NULL, &tv)) <= 0 )
            {
                /* errno is already set if err < 0 */
                if (err == 0)
                    errno = ETIMEDOUT;
                break;
            }

            /* test for success, set errno */
            optlen = sizeof(int);
            if( getsockopt(base->fd, SOL_SOCKET, SO_ERROR, &err, &optlen) < 0 )
                break;
            if( err != 0 )
            {
                errno = err;
                break;
            }
        }
        done = 1;
    }while(0);

    if(done)
        return 0;
    else
        return -1;
}

int ota_send_request(ota_base_t *base)
{
    char host[128] = {0};
    char uri[128] = {0};
    char req[1024] = {0};
    
    ota_get_uri_from_url(base->url, uri);
    ota_get_host_from_url(base->url, host);

    sprintf(req, "GET %s HTTP/1.1\r\n"
            "User-Agent: DOTA/1.0\r\n"
            "Host: %s\r\n"
            "Connection: Keep-Alive\r\n",
            uri, host);
    if(base->download_offset)
    {
        sprintf(req+strlen(req), "Range: bytes=%d-\r\n", base->download_offset);
    }
    strcat(req, "\r\n");

    plog("request:\n%s", req);

    return send(base->fd, req, strlen(req), 0);
}

int ota_get_resp_info(int fd, int *status_code, int *file_size)
{
    char head[1024] = {0};
    char tmp[128] = {0};
    int  offset = 0;
    int  done = 0;
    const char* pos = NULL;

    while(ota_fd_readable(fd, 5000))
    {
        if(recv(fd, head+offset, 1, 0) <= 0)
            break;

        if(head[offset] == '\n')
        {
            if(strstr(head, "\r\n\r\n"))
            {
                done = 1;
                break;
            }
        }
        offset ++;
        if(offset >= (int)sizeof(head) - 1)
        {
            plog("reach head buffer end!!\n");
            break;
        }
    }
    plog("respnse:\n%s", head);

    sscanf(head, "%[^ ] %d", tmp, status_code);

    pos = strcasestr(head, HTTP_CONTENT_LENGTH);
    if(pos)
    {
        pos += strlen(HTTP_CONTENT_LENGTH);
        sscanf(pos, "%d", file_size);
    }
    PRI_INT(*status_code);
    PRI_INT(*file_size);
    return done;
}

int ota_check_updatefile_head(ota_base_t *base)
{
    const int head_len = 256;
    int offset = 0;
    int ret = 0;

    while(ota_fd_readable(base->fd, 10000))
    {
        ret = recv(base->fd, base->updatefile_head+offset, head_len-offset, 0);

        if(ret <= 0)
            break;
        offset += ret;
    }
    if(offset == head_len)
    {
        plog("updatefile check!\n");
#ifndef OTA_TEST
        if(0 == CheckUpdateVersion((UPGRADECHECKSTRUCT*)base->updatefile_head))
            return 0;
#else
        return 0;
#endif
    }
    return -1;
}

static void ota_upload_progress_task(void *para)
{
    web_set_upgrade_state(1, p2p_itoa((long)para));
}

int ota_upload_progress(int percent)
{
    return 0;
    //return ZmdCreateTask(ota_upload_progress_task, (void*)percent, "ota_upload_progress_task");
}

static void ota_upload_failure_task(void *para)
{
    web_set_upgrade_state(2, (char*)para);
}

int ota_on_upgrade_failed(const char* desc)
{
    return 0;
    //return ZmdCreateTask(ota_upload_failure_task, (void*)desc, "ota_upload_progress_task");
}

int ota_save_updatefile_body(ota_base_t *base)
{
    char buf[BUFSIZ];
    int ret = 0;
    int total = 0;
    int percent = 0;

    FILE* file = NULL;
    
    /* "ab" for continue download */
    file = fopen(UPDATEFILE, "ab");

    if(!file)
    {
        plog("open file error:%s\n", UPDATEFILE);
        strcpy(base->failure_desc, "open file error");
        return -1;
    }
    while(ota_fd_readable(base->fd, 10000))
    {
        ret = recv(base->fd, buf, sizeof(buf), 0);

        if(ret <= 0)
        {
            plog("recv socket error!\n");
            break;
        }
        fwrite(buf, 1, ret, file);
        base->updatefile_recv_size+= ret;
        total += ret;
        if(base->updatefile_recv_size*100/base->updatefile_body_size> percent)
        {
            percent = base->updatefile_recv_size*100/base->updatefile_body_size;
            if(percent%5 == 0)
            {
                plog("downloading [%d%%]\n", percent);
                ota_upload_progress(percent);
            }
        }
        if(base->updatefile_recv_size>=base->updatefile_body_size)
        {
            plog("recv done\n");
            break;
        }
#if 0
        static int iiii=0;
        if(base->updatefile_recv_size>1024*1024 && iiii == 0)
        {
            plog("test range bytes!!!\n");
            iiii = 1;
            break;
        }
#endif
    }
    fclose(file);
    return total;
}
int ota_do_upgrade(ota_base_t *base)
{
    plog("ota do upgrade\n");
#ifndef OTA_TEST
    FILE *file = fopen(UPDATEFILE, "rb");
    if(!file || 0 != CheckUpdateFileMD5((UPGRADECHECKSTRUCT*)base->updatefile_head, file))
    {
        plog("check md5 failed!!!\n");
        if(file) fclose(file);
        strcpy(base->failure_desc, "check md5 failed");
        return -1;
    }
    fclose(file);
    p2p_write_upgrade_log();
#ifdef AMBA
    StartUpdate((UpgradeFileHead*)base->updatefile_head);
#else
	UpdateToFlash(((UPGRADECHECKSTRUCT*)base->updatefile_head)->m_filetype);
#endif

#endif
    return 0;
}

int ota_continue_download(ota_base_t *base)
{
    int code = 0;
    int content_size = 0;

    ota_send_request(base);

    if(ota_get_resp_info(base->fd, &code, &content_size))
    {
        if(code/100 != 2)
        {
            sprintf(base->failure_desc, "http return code %d error", code);
            return -1;
        }
        if(base->download_offset == 0)
        {
            /* remove head */
            content_size -= 256;
            base->updatefile_body_size = content_size;
            /* if first download ,we should check head*/
            if(0 != ota_check_updatefile_head(base))
            {
                plog("check update file head failed!\n");
                strcpy(base->failure_desc, "check update file head failed");
                return -1;
            }
            if(base->updatefile_body_size <= 0)
            {
                plog("get content size failed!\n");
                strcpy(base->failure_desc, "get content size failed");
                return -1;
            }
        }
        return  ota_save_updatefile_body(base);
    }
    strcpy(base->failure_desc, "ota_get_resp_info error");
    return -1;
}
int p2p_do_upgrade(const char* url)
{
    ota_base_t base;

    memset(&base, 0, sizeof(base));

    base.url = strdup(url);

    unlink(UPDATEFILE);
    for(int i=0; i<10; i++)
    {
        if((0 == ota_connect_server(&base, 5000)) > 0)
        {
            if(0 > ota_continue_download(&base))
                break;
            if(base.updatefile_recv_size >= base.updatefile_body_size 
                    && base.updatefile_recv_size != 0)
            {        
		if(base.fd > 0)
		{
			close(base.fd);
			base.fd = 0;
		}
                if(0 == ota_do_upgrade(&base))
                {
                    free(base.url);
                    plog("doing upgrade...\n");
                    return 0;
                }
                break;
            }
            if(base.updatefile_recv_size > 0)
                base.download_offset = base.updatefile_recv_size + 256;
            plog("will continue download ....\n");
        }
        else
        {
            plog("connect server failed!\n");
        }
        if(base.res)
        {
            freeaddrinfo(base.res);
            base.res = NULL;
        }
        if(base.fd > 0)
        {
            close(base.fd);
            base.fd = 0;
        }
        sleep(5);
    }
    free(base.url);
    plog("upgrade failed because of %s\n", base.failure_desc);
    ota_on_upgrade_failed(base.failure_desc);
    return 0;
}

#ifdef OTA_TEST
int main(int argc, const char *argv[])
{
    //const char* url = "http://upgrade.meshare.com/update/camera1454485096/updatefile";
    const char* url = "http://upgrade.meshare.com:8000/update/camera1454485096/updatefile";

    p2p_do_upgrade(url);
    return 0;
}
#endif
