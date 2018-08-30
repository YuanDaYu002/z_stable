#ifndef LOG_H
#define LOG_H

#include <syslog.h>
#include <stdio.h>
#include <pthread.h>
#ifdef ANDROID
#include <android/log.h>
#endif

#include "p2pconfig.h" //for SYSLOG
#include "helpfunction.h"

#ifdef SYSLOG 
#undef printf
#define printf(fmt, ...) syslog(LOG_USER|LOG_INFO, fmt, ## __VA_ARGS__)
#endif

#ifdef ANDROID
#define  LOG_TAG    "P2PLIB"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define plog(fmt, ...)  LOGI("%s[%u]%s:" fmt, get_src_name(__FILE__),__LINE__,__FUNCTION__,## __VA_ARGS__)
#define plogfn()  LOGI("%s[%u]%s[%lu]\n", get_src_name(__FILE__),__LINE__, __FUNCTION__, pthread_self())

#else
#define plog(fmt, ...)  printf("%s[%u]%s:" fmt, get_src_name(__FILE__),__LINE__,__FUNCTION__,## __VA_ARGS__)
#define plogfn()  printf("%s[%u]%s[%lu]\n", get_src_name(__FILE__),__LINE__, __FUNCTION__, pthread_self())
#endif




#define plog_str(s) plog( # s " -> %s\n", s)
#define plog_int(v) plog( # v" -> %d\n", v)

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#endif /* end of include guard: LOG_H */
