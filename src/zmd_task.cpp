#include <string>
#include <list>
#include <map>

#include "zmutex.h"
#include "zmdtimer.h"
#include "plog.h"

struct ZmdTaskInfo
{
    zmd_timer_cb_t callback;
    void* para;
};

struct TaskPara
{
	ZMutex * lmutex;
	ZSignal  signal;
	std::string task_name;
	std::list<ZmdTaskInfo> task_list;
};

static ZMutex lmutex;

static void* task_proc(void* para)
{
	TaskPara *named_task = reinterpret_cast<TaskPara *>(para);

    plog("create task[%s] tid[%lu]\n", named_task->task_name.c_str(), pthread_self());

	while(true)
	{
		named_task->signal.Wait();
		std::list<ZmdTaskInfo>::iterator it;
		
		for(;;)
		{
			ZmdTaskInfo task;
			/* {} 是为了让lmutex自动解锁 */
			{
				ZMutexLock l(named_task->lmutex);
				it = named_task->task_list.begin();
				if(it != named_task->task_list.end())
				{
					task = *it;
					named_task->task_list.erase(it);
					it = named_task->task_list.begin();
				}
				else
				{
					break;
				}
			}

			if(task.callback) task.callback(task.para);
		}
	}
    return NULL;
}
static int start_task_proc(void* para)
{
    pthread_t thr;
	
    pthread_create(&thr, NULL, task_proc, para);
    return 0;
}

int ZmdCreateTask(zmd_timer_cb_t task_cb, void* para, const char* task_name)
{
	static std::map<std::string,TaskPara*> named_tasks;

	ZMutexLock l(&lmutex);	  

	if(named_tasks.find(task_name) == named_tasks.end())
	{
		TaskPara *named_task = new TaskPara;

		named_task->task_name = task_name;
		named_task->lmutex = &lmutex;

		named_tasks[task_name] = named_task;
		start_task_proc(named_task);
	}
    ZmdTaskInfo task_info = {task_cb, para};
    named_tasks[task_name]->task_list.push_back(task_info);
	named_tasks[task_name]->signal.Signal();
	return 0;
}
struct DelayTaskPara
{
	zmd_timer_cb_t task_cb;
	void *para;
	std::string task_name;
};
static void DelayTaskTimer(void *para)
{
	DelayTaskPara* task = reinterpret_cast<DelayTaskPara*>(para);
	
	ZmdCreateTask(task->task_cb, task->para, task->task_name.c_str());
	delete task;
}
int ZmdCreateDelayTask(unsigned long msec, zmd_timer_cb_t task_cb, void* para, const char* task_name)
{
	DelayTaskPara *task = new DelayTaskPara;
	task->task_cb = task_cb;
	task->para = para;
	task->task_name = task_name;
	ZmdCreateShortTimer(msec,DelayTaskTimer,task);
	return 0;
}

