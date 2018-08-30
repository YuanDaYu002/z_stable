#ifndef HELPFUNCTION_H
#define HELPFUNCTION_H
#include <time.h>
#include <stdint.h>
#include <string>

#define P2P_COLOR_NONE          "\033[m"  
#define P2P_COLOR_RED           "\033[0;32;31m"  
#define P2P_COLOR_LIGHT_RED     "\033[1;31m"  
#define P2P_COLOR_GREEN         "\033[0;32;32m"  
#define P2P_COLOR_LIGHT_GREEN   "\033[1;32m"  
#define P2P_COLOR_BLUE          "\033[0;32;34m"  
#define P2P_COLOR_LIGHT_BLUE    "\033[1;34m"  
#define P2P_COLOR_DARY_GRAY     "\033[1;30m"  
#define P2P_COLOR_CYAN          "\033[0;36m"  
#define P2P_COLOR_LIGHT_CYAN    "\033[1;36m"  
#define P2P_COLOR_PURPLE        "\033[0;35m"  
#define P2P_COLOR_LIGHT_PURPLE  "\033[1;35m"  
#define P2P_COLOR_BROWN         "\033[0;33m"  
#define P2P_COLOR_YELLOW        "\033[1;33m"  
#define P2P_COLOR_LIGHT_GRAY    "\033[0;37m"  
#define P2P_COLOR_WHITE         "\033[1;37m" 


#define p2p_check_bit(v, i) ((v) & (1<<(i)))
#define p2p_set_bit(v, i)   ((v) |= (1<<(i)))
#define p2p_clear_bit(v, i) ((v) ^= (1<<(i)))

#define P2P_RUN_ONCE\
    static int p2p_run_once_flag = 0;\
    if(p2p_run_once_flag == 0 && (p2p_run_once_flag=1))

#define p2p_malloc(T) (T*)malloc(sizeof(T))
#define p2p_malloc_n(T, n) (T*)malloc(sizeof(T)*(n))

int p2p_gethostbyname(const char* host, char* ip);
const char* p2p_getdatestr();
const char* p2p_gettimestr();
const char* p2p_formattime(time_t *sec, char* timestr, int size);
const char* get_src_name(const char* fpath);

char* get_date4capture(char *date, int date_size, time_t *cur_tm = NULL);


int recvn(int sock, void* buffer, int len);
int get_filesize(const char* fpath);

int p2p_get_filesize(FILE *file);

// 112456
unsigned int p2p_strtime2sec(const char* tm);

// "%Y-%m-%d %H:%M:%S"
unsigned int strdate2sec(const char* date);

std::string p2p_integer_to_string(int i);
std::string p2p_unsigned_to_string(unsigned long i);
std::string p2p_unsigned_long_long_to_string(unsigned long long i);
std::string p2p_float_to_string(float i);

unsigned int p2p_hash(const void *key, size_t length);
//if from 1 to 10 then range is [1, 10)
int p2p_get_rand(int from, int to);

#define p2p_itoa(x) p2p_integer_to_string((x)).c_str()

#define p2p_utoa(x) p2p_unsigned_to_string((x)).c_str()

#define p2p_llutoa(x) p2p_unsigned_long_long_to_string((x)).c_str()

#define p2p_ftoa(x) p2p_float_to_string((x)).c_str()

int p2p_remove_dir(const char *dir);

/* simple thread creation */
#define p2p_long_task(func) p2p_create_thread(func, NULL, #func, 0)
#define p2p_long_task_p(func, para) p2p_create_thread(func, para, #func, 0)

//return pthread_t 
unsigned long p2p_create_thread(void(*func)(void *), void *para, const char *name,
	int want_join);

int p2p_join_thread(unsigned long thread, void **value_ptr);

/* this func ensure @dest is end with '\0' */
char* p2p_strncpy(char* dest, const char* src, size_t n);
/*
	open file with FD_CLOEXEC
*/
FILE* p2p_fopen(const char* fpath, const char* mode);

/*
 * get idle size in KB
 */
unsigned long p2p_get_idle_kbytes(const char* fpath);

/*
 * mode=1, encrypt; mode = 0, decrypt
 */
int p2p_aes_buffer(const char* key, const char* input, int in_size, char* output, int *out_size, int mode);

int p2p_aes_encrypt_buffer(const char* key, const char* input, int in_size, char* output, int *out_size);

int p2p_aes_decrypt_buffer(const char* key, const char* input, int in_size, char* output, int *out_size);

/* 返回系统启动到现在的秒数 */

int p2p_get_uptime();

int p2p_str_is_ip(const char* str);

void p2p_write_string2file(const char* fpath, const char* str);

#endif /* end of include guard: HELPFUNCTION_H */
