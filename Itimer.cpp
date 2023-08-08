


#include <csignal>
#include <sys/time.h>
#include <iostream>
#include "UTError.cpp"

#ifndef _ITIMER
#define _ITIMER

using namespace std;

/**
 * This module provides a class for setting up and managing timers.
 */
class Itimer {
private:
    struct itimerval _timer;
    struct sigaction sa = {0};
    bool _is_initialized = false;


public:
    int TIME_DISABLE_VAL = 0;

    Itimer() {
    }
    
    /**
    * Function to set the handler to the timer
    */
    void set_handler(__sighandler_t handler) { sa.sa_handler = handler; }
    
    /**
    * Connects the sa struct to the timer signal
    */
    void set_time_dom(int dom) const {
        if (sigaction(dom, &sa, NULL) < 0) {
            UTError(SYSTEM_ERROR, TIMER_set_time_ERROR);
            exit(1);
        }
    }

    struct itimerval get_timer() {
        return _timer;
    }

    struct itimerval update_timer_vals(__time_t s_val_time,
                                       __suseconds_t us_val_time,
                                       __time_t s_interval_time,
                                       __suseconds_t us_interval_time) {
        _timer.it_value.tv_sec = s_val_time;        // first time interval, seconds part
        _timer.it_value.tv_usec = us_val_time;        // first time interval, microseconds part

        // configure the timer to expire every 3 sec after that.
        _timer.it_interval.tv_sec = s_interval_time;    // following time intervals, seconds part
        _timer.it_interval.tv_usec = us_interval_time;

        return _timer;
    }

    /**
    * Sets the timer
    */
    void set_timer(int timer_type, struct itimerval *old_value) {
        if (setitimer(timer_type, &_timer, old_value)) {
            UTError(SYSTEM_ERROR, TIMER_ERROR);
            exit(1);
        }
        _is_initialized = true;
    }

    /**
    * Starts a new timer
    */
    void timer_setup(__sighandler_t handler,
                     int dom,
                     __time_t s_val_time,
                     __suseconds_t us_val_time,
                     int timer_type,
                     struct itimerval *old_value) {
        set_handler(handler);
        set_time_dom(dom);
        update_timer_vals(s_val_time, us_val_time, TIME_DISABLE_VAL,
                          TIME_DISABLE_VAL);
        set_timer(timer_type, old_value);
    }
};

#endif
