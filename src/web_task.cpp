#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>

#include "web_task.h"
#include "web.h"
#include "plog.h"

static size_t web_write_mem(void *ptr, size_t size, size_t nmemb, void *data)
{
    //ignore data
    if (!data) return size * nmemb;

    web_buffer_t *buf = (web_buffer_t *)data;

    size_t left_size = buf->size - buf->datalen;
    size_t copy_size = size * nmemb > left_size ? left_size : size * nmemb;

    memcpy(buf->buf + buf->datalen, ptr, copy_size);
    buf->datalen += copy_size;
    return size * nmemb;
}

static size_t web_write_file(void *ptr, size_t size, size_t nmemb, void *stream)
{
    return fwrite(ptr, size, nmemb, (FILE *)stream);
}

WebTask::WebTask()
{
	m_formpost = m_lastptr = NULL;
    m_headerlist = NULL;

	memset(&m_web_buf, 0, sizeof(m_web_buf));

	m_cb = NULL;

	m_file = NULL;

	m_is_getfile = 0;

	m_do_func_called = false;

	m_up.file = NULL;

	_init();
}

WebTask::~WebTask()
{
	if(m_web_buf.buf)
		free(m_web_buf.buf);
	if(m_up.file)
		fclose(m_up.file);
}


void WebTask::SetUrl(const char* url)
{
	plog("%s\n", url);
	curl_easy_setopt(m_curl, CURLOPT_URL, url);
	m_url = url;
}

void WebTask::SetCallback(on_web_done_t cb, void* para)
{
	m_cb = cb;
	m_cb_para = para;
}

int debug_callback(CURL *handle,
        curl_infotype type,
        char *data,
        size_t size,
        void *userptr)
{
    if(type == 0)
        printf("db: %s\n", data);
    return 0;
}

void WebTask::_init()
{
	m_curl = curl_easy_init();

  	/* no progress meter please */
    curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 1L);
    /* connect time out set */
    curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT, 10L);
	/* no signal */
    curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1L);

    curl_easy_setopt(m_curl, CURLOPT_LOW_SPEED_LIMIT, 1024);

    curl_easy_setopt(m_curl, CURLOPT_LOW_SPEED_TIME, 30);

    /*
     * 因为设备刚启动时系统时间不准确，
     * 导致ssl库检查服务器端的证书有效日期会出错
     * 所以先屏蔽对服务器端证书合法性的检查
    */
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L); 

    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 2L); 

    curl_easy_setopt(m_curl, CURLOPT_CAINFO, "/tmp/ca.pem"); 

    curl_easy_setopt(m_curl, CURLOPT_DNS_CACHE_TIMEOUT, 60*60*72);
     
    /*
    curl_easy_setopt(m_curl, CURLOPT_DEBUGFUNCTION, debug_callback);
    curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1L);
    */
}

void WebTask::_add_post_file(const char* item_name, const char* file_path, const char* file_name,  const char* content_type)
{
    curl_formadd(&m_formpost,
                 &m_lastptr,
                 CURLFORM_COPYNAME, item_name,
                 CURLFORM_FILE, file_path,
                 CURLFORM_FILENAME, file_name,
                 CURLFORM_CONTENTTYPE, content_type,
                 CURLFORM_END);	
}
static size_t my_read_func(void *ptr, size_t size, size_t nmemb, void *para)
{
	file_upload_para_t* p = (file_upload_para_t*)para;

	if(!p->file)
	{
		p->file = fopen(p->filepath, "rb");
		if(!p->file)
			return 0;
	}
	int need = size*nmemb;

	//plog("need----%d\n", need);
	int ret = fread(ptr, 1, need, p->file);
	plog("ret----%d\n", ret);
	if(ret > 0)
	{
		if(ret < need)
		{			
			clearerr(p->file);
			usleep(1000*1000);
		}
	}
	else
	{
		if(p->file)
		{
			fclose(p->file);
			p->file = NULL;
		}
		plog("read file [%s] end!\n", p->filepath);
	}
	
	return ret;
}
void WebTask::_add_post_file_chunked(const char* item_name, const char* file_path, const char* file_name,  const char* content_type)
{
	m_headerlist  = curl_slist_append(m_headerlist, "Transfer-Encoding: chunked"); 
	curl_easy_setopt(m_curl, CURLOPT_HEADER, 0L);  
	curl_easy_setopt(m_curl, CURLOPT_POST, 1L);  
	curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1L);  
	curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1L);  
	// setup chunk and post data  
	curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_headerlist);  
	/* we want to use our own read function */
	curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, my_read_func);
	
	curl_formadd(&m_formpost,
		&m_lastptr,
		CURLFORM_COPYNAME, item_name,
		CURLFORM_FILENAME, file_name,
		CURLFORM_STREAM, &m_up,
		CURLFORM_CONTENTTYPE, content_type,
		CURLFORM_END); 
	m_up.file = NULL;
	strcpy(m_up.filepath, file_path);
}

void WebTask::AddPostString(const char* item_name, const char* item_data)
{
	plog("%s:%s\n", item_name, item_data);
    curl_formadd(&m_formpost,
                 &m_lastptr,
                 CURLFORM_COPYNAME, item_name,
                 CURLFORM_COPYCONTENTS, item_data,
                 CURLFORM_END); 
}

void WebTask::AddPostPicture(const char* item_name, const char* file_path, const char* file_name)
{
	AddPostFile(item_name, file_path, file_name, "image/jpeg");
}
void WebTask::AddPostFile(const char* item_name, const char* file_path, const char* file_name, const char* content_type )
{	
	plog("%s:%s\n", item_name, file_path);
	if( access(file_path, F_OK) != 0 )
		return;
	
	if(!file_name)
	{
		file_name = strrchr(file_path, '/');

		if(!file_name) 
			file_name = get_src_name(file_path);
		else
			file_name += 1;
	}
	_add_post_file(item_name, file_path, file_name, content_type );
}
void WebTask::AddPostFileChunked(const char* item_name, const char* file_path, const char* file_name , const char* content_type )
{
	plog("%s:%s\n", item_name, file_path);
	if( access(file_path, F_OK) != 0 )
		return;
	_add_post_file_chunked(item_name, file_path, file_name, content_type );
}
int WebTask::DoGetString()
{
	if(m_formpost)
		curl_easy_setopt(m_curl, CURLOPT_HTTPPOST, m_formpost);

    /* send all data to this function  */
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, web_write_mem);

    /* we want the headers be written to this file handle */
    curl_easy_setopt(m_curl, CURLOPT_WRITEHEADER, NULL);

	m_web_buf.size = 200*1024;
	m_web_buf.buf = (char*)malloc(m_web_buf.size);	
	m_web_buf.datalen = 0;
	memset(m_web_buf.buf, 0, m_web_buf.size);
	
    /* we want the body be written to this file handle instead of stdout */
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &m_web_buf);

	m_do_func_called = true;
	return 0;
}

extern int p2p_network_ok();

int WebTask::WaitTaskDone()
{
	    /* get it! */ 
    CURLcode res = curl_easy_perform(m_curl);

    /* Check for errors */ 
    if(res != CURLE_OK)
    {
        plog("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

       if(0 == strncmp("Couldn't resolve host name", curl_easy_strerror(res), strlen("Couldn't resolve host name")))
        {
            if(p2p_network_ok())
            {
                printf("\n\n touch /tmp/config/resolv.conf\n\n");
                system("touch /tmp/config/resolv.conf");
            }
            else
            {
                printf("\n\n p2p_network_ok() is not ok now\n\n");
            }
        }
    }
	res = (CURLcode)_on_work_done((int)res);

	//clean up
	
	if(m_formpost)
		curl_formfree(m_formpost);
    /* free slist */
	if(m_headerlist)
		curl_slist_free_all(m_headerlist);
		
	curl_easy_cleanup(m_curl);

	return (int)res;
}
int WebTask::DoGetFile(const char* range) {
  if (m_formpost)
    curl_easy_setopt(m_curl, CURLOPT_HTTPPOST, m_formpost);

  if (range)
    curl_easy_setopt(m_curl, CURLOPT_RANGE, range);


  char fpath[128];

  strcpy(fpath, "/tmp/XXXXXX");

  int fd = mkstemp(fpath);
  if (fd < 0) {
    return -1;
  }
//  close(fd);
  plog("file will save as %s\n", fpath);

  fcntl(fd, F_SETFD, FD_CLOEXEC);

  //m_file = p2p_fopen(fpath, "wb");
  m_file = fdopen(fd, "wb");
  if (!m_file) {
    plog("create file :%s error!!!\n", fpath);
    return -1;
  }
  m_is_getfile = 1;


  m_filepath = fpath;

  /* send all data to this function  */
  curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, web_write_file);

  /* we want the headers be written to this file handle */
  curl_easy_setopt(m_curl, CURLOPT_WRITEHEADER, NULL);

  /* we want the body be written to this file handle instead of stdout */
  curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, m_file);
  // for 302
  curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1);

  m_do_func_called = true;
  return 0;
}

int WebTask::_on_work_done(int result)
{
	if(m_is_getfile == 0 && m_web_buf.buf)
	{
		if(m_web_buf.buf)
		{
			if(m_web_buf.datalen > 0)
				m_web_buf.buf[m_web_buf.size-1] = '\0';
			std::string get_string = m_web_buf.buf;
			if(m_cb) m_cb(result, get_string, m_cb_para);
		}			
	}
	else if(m_is_getfile)
	{
		double filesize = 0;
		curl_easy_getinfo(m_curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &filesize);
		plog("filesize:%f\n", filesize);
		if(m_file)	
			fclose(m_file);
		if(result != 0)
			unlink(m_filepath.c_str());
		if(get_filesize(m_filepath.c_str()) != (long)filesize)
		{
			unlink(m_filepath.c_str());
			result = -1;
		}
		if(m_cb) 
		{
			m_cb(result, m_filepath, m_cb_para);
		}			
	}
	return result;
}
const char*  WebTask::GetResultString()
{
	if(m_is_getfile == 0 && m_web_buf.buf)
		return m_web_buf.buf;
	else
		return NULL;
}

const char*  WebTask::GetFilePath()
{
	if(m_is_getfile == 1)
		return m_filepath.c_str();
	else
		return NULL;
}

void WebTask::SetConnectTimeout(int timeo)
{
    plog("[%d]\n", timeo);
    /* connect time out set */
    curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT, timeo);
}


