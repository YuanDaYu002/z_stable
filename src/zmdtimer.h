#ifndef ZMDTIMER_H
#define ZMDTIMER_H

typedef void (*zmd_timer_cb_t)(void*);
/************************************
* create a timer, and run once
* @msec, in million seconds,  after this interval, timer will be trigger
* @id, timer id
* @timer_fun, call back function
* @para, para for call back function
*/

//this timer should finish job in no time
int ZmdCreateShortTimer(unsigned long msec, zmd_timer_cb_t timer_cb, void* para);

//this timer handle jobs that can be delay
int ZmdCreateTimer(unsigned long msec, zmd_timer_cb_t timer_cb, void* para);

//this timer create a named timer, and all timers with this name work in the same one thread
int ZmdCreateNamedTimer(unsigned long msec, zmd_timer_cb_t timer_cb, 
								void* para, const char* timer_name);

#endif /* end of include guard: ZMDTIMER_H */
