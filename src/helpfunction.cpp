#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/vfs.h>

#include "aes.h"
#include "helpfunction.h"
#include "plog.h"
#include "jenkins_hash.h"

int p2p_gethostbyname(const char* host, char* ip)
{
    struct addrinfo hints;
    struct addrinfo *res, *cur;
    int ret;
    struct sockaddr_in *addr;
    char ipbuf[16];

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; /* Allow IPv4 */
    hints.ai_flags = 0; 
    hints.ai_protocol = 0; /* Any protocol */
    hints.ai_socktype = SOCK_STREAM;

    ret = getaddrinfo(host, NULL,&hints,&res);

    if (ret != 0)
    {
        perror("getaddrinfo");
        return -1;
    }

    for (cur = res; cur != NULL; cur = cur->ai_next) {
        addr = (struct sockaddr_in *)cur->ai_addr;
		sprintf(ip, "%s", inet_ntop(AF_INET, &addr->sin_addr, ipbuf, 16));
        break;
    }
    freeaddrinfo(res);
    return ret;
}

const char* p2p_getdatestr()
{
    static char timestr[32];
    time_t t;
    struct tm nowtime;

    time(&t);
    localtime_r(&t, &nowtime);
    strftime(timestr,sizeof(timestr),"%Y-%m-%d %H:%M:%S",&nowtime);
    return timestr;
}
const char* p2p_gettimestr()
{
    static char timestr[32];
    struct tm nowtime;
    struct timeval tv;
    gettimeofday(&tv, NULL);

    localtime_r(&tv.tv_sec, &nowtime);
    sprintf(timestr, "%02d:%02d:%02d.%03ld", nowtime.tm_hour, nowtime.tm_min, 
            nowtime.tm_sec, tv.tv_usec/1000);
    return timestr;
}

const char* p2p_formattime(time_t *sec, char* timestr, int size)
{
    struct tm nowtime;

    localtime_r(sec, &nowtime);
    strftime(timestr,size,"%Y-%m-%d %H:%M:%S",&nowtime);
    return timestr;
}


int recvn(int sock, void* buffer, int len)
{
    int retLen = 0;
    int total = 0;
    while (total<len)
    {
        retLen = recv(sock,(char*)buffer+total,len-total,0);
        if(retLen <= 0)
        {
            return retLen;
        }
        total+=retLen;
    }
    return total;
}
int get_filesize(const char* fpath)
{
    struct stat st;

    memset(&st, 0, sizeof(struct stat));
    stat(fpath, &st);
    return st.st_size;
}

int p2p_get_filesize(FILE *file)
{
    struct stat st;

    memset(&st, 0, sizeof(struct stat));
    fstat(fileno(file), &st);
    return st.st_size;
}

const char* get_src_name(const char* fpath)
{
    const char* pos = strrchr(fpath, '/');
    if(pos) return pos+1;
    return fpath;
}
char* get_date4capture(char *date, int date_size, time_t *cur_tm )
{
	time_t t;
	struct tm result;

	if(cur_tm)
		t = *cur_tm;
	else
		time(&t);
	localtime_r(&t, &result);

	strftime(date, date_size, "%Y-%m-%d-%H-%M-%S", &result);
	return date;
}

unsigned int p2p_strtime2sec(const char* tm)
{
    //23:10:46  hour:23, min:10, sec:46
    //return 23*3600 + 10*60 + 46
    int hour=0;
    int min=0;
    int sec=0;

    sscanf(tm, "%02d:%02d:%02d", &hour, &min, &sec);
    return hour*3600+min*60+sec;
}

unsigned int strdate2sec(const char* date)
{
	// 2015-07-21
	struct tm tm;
	char format[32];
	
	memset(&tm, 0, sizeof(struct tm));
	
	if(strlen(date) <= 10)
		strcpy(format, "%Y-%m-%d");
	else
		strcpy(format, "%Y-%m-%d %H:%M:%S");
	
	if(strptime(date, format, &tm))
		return mktime(&tm);
	else
		return 0;
}

std::string p2p_integer_to_string(int i)
{
    char buf[128] = {0};

    snprintf(buf, sizeof(buf)-1, "%d", i);
    return buf;
}
std::string p2p_float_to_string(float i)
{
    char buf[128] = {0};

    snprintf(buf, sizeof(buf)-1, "%f", i);
    return buf;
}

std::string p2p_unsigned_to_string(unsigned long i)
{
    char buf[128] = {0};

    snprintf(buf, sizeof(buf)-1, "%lu", i);
    return buf;
}

std::string p2p_unsigned_long_long_to_string(unsigned long long i)
{
	char buf[128] = {0};

	snprintf(buf, sizeof(buf)-1, "%llu", i);
	return buf;
}
unsigned int p2p_hash(const void *key, size_t length)
{
    return jenkins_hash(key, length);
}

int p2p_get_rand(int from, int to)
{
    srand(time(NULL));

    return rand()%(to-from) + from;
}
int p2p_remove_dir(const char *dir)
{
	plog("%s\n", dir);
	
	DIR* pdir = NULL;
	struct dirent * pdirent = NULL;
	std::string dir_path;

	pdir = opendir(dir);
	if(!pdir) return -1;

	dir_path = dir;
	if(dir[strlen(dir)-1] != '/')
		dir_path +="/";
	
	while((pdirent = readdir(pdir))!= NULL)
	{
		if(strcmp(pdirent->d_name, ".") && strcmp(pdirent->d_name, ".."))
		{
			std::string tmp = dir_path + std::string(pdirent->d_name);
			struct stat st;
			stat(tmp.c_str(), &st);
			if(!S_ISDIR(st.st_mode))
				unlink(tmp.c_str());
			else
				p2p_remove_dir(tmp.c_str());
		}
	}
	closedir(pdir);
	rmdir(dir);
	return 0;	
}


typedef void(*p2p_thread_func_t)(void *);

struct p2p_thread_para_t
{
	char *name;
	void *para;
	p2p_thread_func_t cb;
	pthread_t thread;
	int want_join;
};
static void* p2p_thread_func(void *para)
{
	p2p_thread_para_t *pth = (p2p_thread_para_t *)para;

	if(!pth->want_join)
		pthread_detach(pth->thread);
	plog("create thread[%s %lu]\n", pth->name, pth->thread);
    printf("%s %d %s tid:[%d] pid:[%d] ppid:[%d]\n", __FILE__,__LINE__,__FUNCTION__,(int)pthread_self(),(int)getpid(),(int)getppid());

	pth->cb(pth->para);
	
	plog("thread[%s %lu] exit\n", pth->name, pth->thread);

	free(pth->name);
	free(pth);

	return NULL;
}
unsigned long p2p_create_thread(p2p_thread_func_t func, void *para, const char *name, int want_join)
{	
	p2p_thread_para_t *pth = (p2p_thread_para_t *)malloc(sizeof(p2p_thread_para_t));

	pth->cb = func;
	pth->para = para;
	pth->name = strdup(name);
	pth->want_join = want_join;
	
    if(pthread_create(&pth->thread, NULL, p2p_thread_func, pth))
    {
		free(pth->name);
		free(pth);
		plog("create thread [%s] failed because [%s]\n", name, strerror(errno));
		return 0;
    }
 #if 0 
	pthread_attr_t thread_attr;
	pthread_attr_init(&thread_attr);
	size_t stack_size = 0;

	pthread_attr_getstacksize(&thread_attr, &stack_size);

	stack_size = 4 * 1024 * 1024;
	pthread_attr_setstacksize(&thread_attr, stack_size);
	
    if(pthread_create(&pth->thread, &thread_attr, p2p_thread_func, pth))
    {
		free(pth->name);
		free(pth);
		plog("create thread [%s] failed because [%s]\n", name, strerror(errno));
             pthread_attr_destroy(&thread_attr);
		return 0;
    }
    pthread_attr_destroy(&thread_attr);
 #endif
	return pth->thread;
}
int p2p_join_thread(unsigned long thread, void **value_ptr)
{
	return pthread_join(thread, value_ptr);
}

char* p2p_strncpy(char* dest, const char* src, size_t n)
{
	snprintf(dest, n, "%s", src);
	return dest;
}

FILE* p2p_fopen(const char* fpath, const char* mode)
{
	FILE *file = fopen(fpath, mode);
	
	if(file) fcntl(fileno(file), F_SETFD, FD_CLOEXEC);

	return file;
}

unsigned long p2p_get_idle_kbytes(const char* fpath)
{
    struct statfs stbuf;
    char dir[128] = {0};

    if(fpath)
    {
        strcpy(dir, fpath);
        while(strlen(dir) && access(dir, F_OK)!=0)
        {
            char * pos = strrchr(dir, '/');

            if(pos == dir + strlen(dir))
                *pos = '\0';
            else if(pos)
                *(pos+1) = '\0';
            else 
                return 0;
        }
    }
    else
    {
        return 0;
    }

    plog("%s\n", dir);
    if(strlen(dir) == 0)
        return 0;
    if(0 == statfs(dir, &stbuf))
    {
        if(stbuf.f_bsize < 1024)
            return stbuf.f_bsize*stbuf.f_bfree;
        else
            return (stbuf.f_bsize/1024)*stbuf.f_bfree;
    }
    else
    {
        return 0;
    }
}

int p2p_get_uptime()
{
    struct timespec time_start={0, 0};

    clock_gettime(CLOCK_MONOTONIC, &time_start);

    return time_start.tv_sec;
}

int p2p_aes_buffer(const char* key, const char* input, int in_size, char* output, int *out_size, int mode)
{
    mbedtls_aes_context ctx; 

    mbedtls_aes_init( &ctx );  

    int block_number = in_size/16;
    int remainder = in_size%16;

    if(*out_size%16 != 0)
    {
        plog("output size must %% 16==0\n");
        return -1;
    }

    if(remainder)
        in_size+= 16-remainder;

    if(in_size > *out_size)
    {
        plog("output buffer size is not enough!\n");
        return -1;
    }
    *out_size = in_size;

    if(mode == 1)
    {
        mode = MBEDTLS_AES_ENCRYPT;
        mbedtls_aes_setkey_enc( &ctx, (unsigned char*)key, strlen(key)*8); 
    }
    else
    {
        mode = MBEDTLS_AES_DECRYPT;
        mbedtls_aes_setkey_dec( &ctx, (unsigned char*)key, strlen(key)*8); 
    }

    int i = 0;
    for(; i<block_number; i++)
    {
        mbedtls_aes_crypt_ecb( &ctx, mode, (unsigned char*)input+i*16, (unsigned char*)output+i*16 ); 
    }

    if(remainder)
    {
        unsigned char buf[16];

        memset(buf, 0, sizeof(buf));

        memcpy(buf, input+i*16, remainder);

        mbedtls_aes_crypt_ecb( &ctx, mode, buf, (unsigned char*)output+i*16 );
    }
    mbedtls_aes_free( &ctx );
    return 0;
}

int p2p_aes_decrypt_buffer(const char* key, const char* input, int in_size, char* output, int *out_size)
{
    return p2p_aes_buffer(key, input, in_size, output, out_size, 0);
}

int p2p_aes_encrypt_buffer(const char* key, const char* input, int in_size, char* output, int *out_size)
{
    return p2p_aes_buffer(key, input, in_size, output, out_size, 1);
}
int p2p_str_is_ip(const char* str)
{
	for(;*str;)
	{
		if(*str != '.' && *str != ':' && (*str >'9' || *str<'0'))
			return 0;
		str++;
	}
	return 1;
}
void p2p_write_string2file(const char* fpath, const char* str)
{
    FILE* file = fopen(fpath, "w");

    if(file)
    {
        if(str)
            fwrite(str, 1, strlen(str), file);
        fclose(file);
    }
}
