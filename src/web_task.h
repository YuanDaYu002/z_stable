#ifndef WEB_ASYNC_TASK_HHHHHH_
#define WEB_ASYNC_TASK_HHHHHH_

#include <curl.h>
#include <string>


typedef struct _web_buffer_t {
    char* buf;
    //buffer orignal size
    int   size;
    //length of data that wrote to buffer
    int   datalen;
} web_buffer_t;

struct file_upload_para_t
{
	char filepath[128];
	FILE* file;
};
// web的回调函数
// 第一个参数是结果， 0 成功， 否则失败
// 第二个参数，DoGetString 时，是 web返回的文本内容； DoGetFile时，是下载的文件的路径
// 第三个参数，用户自定义参数
typedef void (*on_web_done_t)(int, const std::string&, void*);

class WebTask
{
 public:
 	WebTask();
	~WebTask();
	

	void SetUrl(const char* url);

	void SetCallback(on_web_done_t cb, void* para = NULL);

	void AddPostString(const char* item_name,  const char* item_data);
	
	void AddPostPicture(const char* item_name, const char* file_path, const char* file_name = NULL);

	void AddPostFile(const char* item_name, const char* file_path, const char* file_name = NULL, const char* content_type = "application/octet-stream");
	void AddPostFileChunked(const char* item_name, const char* file_path, const char* file_name = NULL, const char* content_type = "application/octet-stream");
	
	// must call DoGetString() or DoGetFile() before start task
	int  DoGetString();
	// must call DoGetString() or DoGetFile() before start task
	// range like "0-255"
	int  DoGetFile(const char* range=NULL);
	
	int  WaitTaskDone();

	const char*  GetResultString();
	const char*  GetFilePath();

    /**
     * @timeo seconds
     */
    void SetConnectTimeout(int timeo);

	
	CURL  *m_curl;
	//CURLM *m_curl_m; 

	int    m_is_getfile;

	// if call DoGetString or DoGetFile
	bool   m_do_func_called;
	
	//for post
	struct curl_httppost *m_formpost;
    struct curl_httppost *m_lastptr;
    struct curl_slist *m_headerlist;
	on_web_done_t m_cb;
	void* m_cb_para;

	//for download file
	FILE* m_file;
	std::string m_filepath;

	int   m_web_result;

	//for tracer
	std::string m_url;

	file_upload_para_t m_up;

 private:
	void _init();
	void _add_post_file(const char* item_name, const char* file_path, const char* file_name,  const char* content_type);
	void _add_post_file_chunked(const char* item_name, const char* file_path, const char* file_name,  const char* content_type);
	
	int _on_work_done(int result);

	web_buffer_t m_web_buf;

 private:
 	WebTask(const WebTask&);
	WebTask& operator=(const WebTask&);
};
#endif

