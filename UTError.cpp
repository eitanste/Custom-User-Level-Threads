#include <iostream>
#include <string>

#ifndef _UTError
#define _UTError

const std::string SYSTEM_ERROR = "System error: ";
const std::string LIBRARY_ERROR = "thread library error: ";
// error types
const std::string QUANTUM_TOO_SMALL = "quantum time should be a positive "
									 "number";
const std::string ALLOCATION_ERROR = "Could not allocate the asked memory";
const std::string TIMER_START_ERROR = "Timer is not initialized.";
const std::string TIMER_ERROR = "There is a problem in getting the timer time";
const std::string TIMER_set_time_ERROR = "An error while setting sigaction "
                                         "in timer";
const std::string TID_OUT_OF_RANGE_ERROR = "The passed TID is incorrect, "
                                           "it should be greater then 0 and "
                                           "less then MAX_THREAD_NUM";
const std::string EMPTY_TID_ERROR = "The passed TID it incorrect, no thread"
                                    " with such TID";
const std::string NO_THREADS_AVAILABLE = "No more threads are possible - "
										 "full capacity";
const std::string NO_ENTRYPOINT = "The provided entry point is null - thread "
                                  "was not created";
const std::string NEGATIVE_SLEEPING_TIME = "Sleeping must be only for a"
                                           "natural number of quantums";
const std::string SLEEP_THREAD0 = "It is impossible to call uthread_sleep on"
                                  " thread 0";
const std::string BLOCK_THREAD0 = "It is impossible to call uthread_block on"
								  " thread 0";
const std::string SIGSET_ERROR = "Error occurred while trying to save the "
                                 "thread state";

class UTError {
  public:
	UTError(const std::string& type, const std::string& msg) {
		std::cerr << type << msg << std::endl;
	}
};

#endif