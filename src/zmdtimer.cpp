
#include <list>
#include <map>
#include <string>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "zmdtimer.h"
#include "zmutex.h"

struct ZmdTimerInfo
{
    //int id;
    zmd_timer_cb_t callback;
    void* para;
    struct timeval create_time;
	//use for when the system time be changed by accident, 
	//and the time is earlier than create time, we trigger at once
	//miliiseconds from create time to trigger time
	unsigned long interval;	
};
struct TimerPara
{
	//timer info
	ZMutex * lmutex;
	unsigned long sleep_time;	
	char timer_name[32];
	std::list<ZmdTimerInfo> timers;
	bool loop;
};
#define DEFAULT_SLEEP_TIME  1000*1000

static ZMutex lmutex;

static void* timer_proc(void* para)
{
	printf("%s %d %s tid:[%d] pid:[%d] ppid:[%d]\n", __FILE__,__LINE__,__FUNCTION__,(int)pthread_self(),(int)getpid(),(int)getppid());
	pthread_detach(pthread_self());
    struct timeval cur_time;

	TimerPara * ptimer = (TimerPara*)para;

	printf("%s timer_proc tid:%lu\n", ptimer->timer_name, pthread_self());

    do
    {
		if(ptimer->sleep_time)
			usleep(ptimer->sleep_time);
		if(ptimer->lmutex)
			ptimer->lmutex->Lock();
        std::list<ZmdTimerInfo> time_up_timers;
        std::list<ZmdTimerInfo>::iterator iter = ptimer->timers.begin();
        gettimeofday(&cur_time, NULL);
        for(; iter!= ptimer->timers.end();)
        {
            if( abs(cur_time.tv_sec - iter->create_time.tv_sec) > iter->interval/1000
				|| ( abs(cur_time.tv_sec - iter->create_time.tv_sec) == iter->interval/1000
				&&   cur_time.tv_usec - iter->create_time.tv_usec > (iter->interval%1000 )*1000) )
            {
				time_up_timers.push_back(*iter);
            	ptimer->timers.erase(iter++);
            }
			else
				iter++;
        }
		if(ptimer->lmutex)
			ptimer->lmutex->Unlock();
        if(time_up_timers.size())
        {
            std::list<ZmdTimerInfo>::iterator iter = time_up_timers.begin();
            for(;iter != time_up_timers.end();iter++)
            {	
                iter->callback(iter->para);
            }
        }
    }while(ptimer->loop);
	delete ptimer;
    return NULL;
}

static int start_timer_proc(void* para)
{
    pthread_t thr;
	
    pthread_create(&thr, NULL, timer_proc, para);
    return 0;
}

int ZmdCreateShortTimer(unsigned long msec, zmd_timer_cb_t timer_cb, void* para)
{
	return ZmdCreateNamedTimer(msec, timer_cb, para, "short timer");
}
int ZmdCreateTimer(unsigned long msec, zmd_timer_cb_t timer_cb, void* para)
{
    return ZmdCreateNamedTimer(msec, timer_cb, para, "default timer");
}

int ZmdCreateNamedTimer(unsigned long msec, zmd_timer_cb_t timer_cb, 
								void* para, const char* timer_name)
{
	static std::map<std::string,TimerPara*> named_timers;
	
	ZMutexLock l(&lmutex);	  

	if(named_timers.find(timer_name) == named_timers.end())
	{
		TimerPara *named_timer = new TimerPara;

		named_timer->sleep_time = DEFAULT_SLEEP_TIME;
		strcpy(named_timer->timer_name, timer_name);
		named_timer->loop = true;
		named_timer->lmutex = &lmutex;

		named_timers[timer_name] = named_timer;
		start_timer_proc(named_timer);
	}
	
    struct timeval create_time;
    gettimeofday(&create_time, NULL);

    ZmdTimerInfo timer_info = {timer_cb, para, create_time, msec};
    named_timers[timer_name]->timers.push_back(timer_info);

    if(named_timers[timer_name]->sleep_time/1000 > msec && msec != 0)
    {
		if(msec < 100) msec = 100;
		
		named_timers[timer_name]->sleep_time = msec*1000;
    }
	return 0;
}

