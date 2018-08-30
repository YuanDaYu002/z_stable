#ifndef ZMUTEX_H
#define ZMUTEX_H

#include <pthread.h>
#include <assert.h>
#include <stdlib.h>      // for abort()
#include <stdio.h>

class ZMutex {
 public:
  inline ZMutex();
  inline ~ZMutex();
  inline void Lock();    // Block if needed until free then acquire exclusively
  inline void Unlock();  // Release a lock acquired via Lock()
 private:
  pthread_mutex_t mutex_;
  volatile bool is_safe_;
  inline void SetIsSafe() { is_safe_ = true; }

  ZMutex(ZMutex* /*ignored*/) {}
  ZMutex(const ZMutex&);
  void operator=(const ZMutex&);
};

#define SAFE_PTHREAD(fncall)  do {   /* run fncall if is_safe_ is true */  \
  if (is_safe_ && fncall(&mutex_) != 0) { printf("%s lock error!!!\n", __FUNCTION__);abort();}                           \
} while (0)

ZMutex::ZMutex()             {
  SetIsSafe();
  if (is_safe_ && pthread_mutex_init(&mutex_, NULL) != 0) 
  {
      printf("%s mutex init failed!!!\n", __FUNCTION__);
      abort();
  }
}
ZMutex::~ZMutex()            { SAFE_PTHREAD(pthread_mutex_destroy); }
void ZMutex::Lock()         { SAFE_PTHREAD(pthread_mutex_lock); }
void ZMutex::Unlock()       { SAFE_PTHREAD(pthread_mutex_unlock); }
// --------------------------------------------------------------------------
// Some helper classes

// ZMutexLock(mu) acquires mu when constructed and releases it when destroyed.
class ZMutexLock {
 public:
  explicit ZMutexLock(ZMutex *mu) : mu_(mu) { mu_->Lock(); }
  ~ZMutexLock() { mu_->Unlock(); }
 private:
  ZMutex * const mu_;
  // Disallow "evil" constructors
  ZMutexLock(const ZMutexLock&);
  void operator=(const ZMutexLock&);
};

// Catch bug where variable name is omitted, e.g. ZMutexLock (&mu);
#define ZMutexLock(x) COMPILE_ASSERT(0, mutex_lock_decl_missing_var_name)


class ZSignal
{
public:
	ZSignal():m_signaled(false)
	{
		pthread_mutex_init(&m_mutex, NULL);		
		pthread_condattr_init(&m_condattr);
#ifndef ANDROID_API_LEVEL_LOW
		pthread_condattr_setclock(&m_condattr, CLOCK_MONOTONIC);
#endif
		pthread_cond_init(&m_cond, &m_condattr);
	}
	~ZSignal()
	{
		pthread_condattr_destroy(&m_condattr);
		pthread_mutex_destroy(&m_mutex);
		pthread_cond_destroy(&m_cond);
	}

	void Wait()
	{ 
		pthread_mutex_lock(&m_mutex);
		if(!m_signaled)
			pthread_cond_wait(&m_cond, &m_mutex); 
		m_signaled = false;
		pthread_mutex_unlock(&m_mutex);
	}
	void Wait(long ms)
	{ 
		struct timespec tv;
		clock_gettime(CLOCK_MONOTONIC, &tv);
		tv.tv_sec += ms/1000;
		tv.tv_nsec += (ms%1000)*1000*1000;
		pthread_mutex_lock(&m_mutex);
		if(!m_signaled)
			pthread_cond_timedwait(&m_cond, &m_mutex, &tv); 
		m_signaled = false;
		pthread_mutex_unlock(&m_mutex);
	}
	void Signal() 
	{
		pthread_mutex_lock(&m_mutex);
		m_signaled = true;
		pthread_cond_signal(&m_cond); 
		pthread_mutex_unlock(&m_mutex);		
	}
private:
	pthread_mutex_t 	m_mutex;
	pthread_cond_t 		m_cond;
	pthread_condattr_t  m_condattr;
	volatile bool 		m_signaled;
};

#endif  
