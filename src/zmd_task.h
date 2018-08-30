#ifndef _ZMD_TASK_H_2015_12_01_
#define _ZMD_TASK_H_2015_12_01_

//!!! one task name is one thread!!!
int ZmdCreateTask(zmd_timer_cb_t task_cb, void* para, const char* task_name);

//!!! one task name is one thread!!!
int ZmdCreateDelayTask(unsigned long msec, zmd_timer_cb_t task_cb, void* para, const char* task_name);

#endif
