#include <deque>
#include "uthreads.h"
#include "UTError.cpp"
#include "UThread.cpp"
#include "Itimer.cpp"
#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdbool.h>
#include <bits/stdc++.h>

// return flags
int FAILED = -1;
int SUCCESS = 0;
const int SAVE_MASK = 1;

int _quantum_usecs;
// min heap for free indexes
std::priority_queue<unsigned int, std::vector<unsigned int>,
        std::greater<unsigned int>> indexes;
// all threads
UThread *threads[MAX_THREAD_NUM] = {nullptr};
// ready deque
std::deque<UThread *> ready_queue;
// Blocked hashmap
std::vector<UThread *> blocked_threads;
// terminated
std::vector<UThread *> terminated;
// running index
UThread *running_thread;
// timer
Itimer timer = Itimer();
int _ran_quantum = 1;
// signal handling
sigset_t signal_set;
int TIME_DISABLE = 0;


int LONG_JUMP_VAL = 152; // Random value not in (-1, 1, 0)



int MAIN_THREAD = 0;
#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr) {
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5


/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr) {
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
                 "rol    $0x9,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}


#endif

#define SECOND 1000000
#define STACK_SIZE 4096

/**
* Makes the needed action to the previous running thread
*/
void thread_handler_by_state(ProcessState &new_state) {
    switch (new_state) {
        case BLOCKED:
            blocked_threads.push_back(running_thread);
            break;

        case RUNNING:
            running_thread->run_thread();
            break;

        case READY:
            ready_queue.push_back(running_thread);
            break;

        case TERMINATED:
            running_thread->stop_thread(TERMINATED);
            int tid = running_thread->get_TID();
            terminated.push_back(threads[tid]);
            threads[tid] = nullptr;
            running_thread = nullptr;
            indexes.push(tid);
            break;
    }
}

/**
* Updates the sleeping thread by decreasing the number of still needed 
* sleep quantums and returning them to ready queue if needed
*/ 
void update_sleeping_threads() {
    if (!blocked_threads.empty()) {
        for (long unsigned int i = 0 ; i < blocked_threads.size() ; ++i) {
            UThread *blocked_thread = blocked_threads[i];
            int num_of_quantum_left = blocked_thread->get_num_quantums();
            if (num_of_quantum_left > 0) {
                blocked_thread->set_num_quantums(num_of_quantum_left - 1);
                if (blocked_thread->get_num_quantums() == 0) {
                    blocked_thread->set_num_quantums(-1);
                    // set ready status
                    if (!blocked_thread->get_blocked()) {
                        blocked_thread->set_state(READY);
                        //remove from blocked
                        blocked_threads.erase(blocked_threads.begin() + i);
                        // add to end of ready
                        ready_queue.push_back(blocked_thread);
                    }
                }
            }
        }
    }
}


/**
* Frees the allocated memory and exits with the needed signal
*/
void end_program(int signal) {
    // Disconnect threads from using lists
    running_thread = nullptr;
    for (long unsigned int i = 0; i < ready_queue.size() ; i++) {
        ready_queue[i] = nullptr;
    }

    for (long unsigned int i = 0; i < blocked_threads.size() ; i++) {
        blocked_threads[i] = nullptr;
    }

    for (long unsigned int i = 0; i < terminated.size() ; i++) {
        terminated[i] = nullptr;
    }

    // Delete the treads
    int index = 0;
    for (UThread * thread : threads) {
        if (thread == nullptr) {
            ++index;
            continue;
        }
        if (thread->get_TID() == 0) {
            ++index;
            continue;
        }
        delete thread;
        threads[index] = nullptr;
        ++index;
    }
    exit(signal);
}

void scheduler();

/**
* Switches the running thread with the next one
*/
void switch_threads(ProcessState new_state) {
    thread_handler_by_state(new_state);
    running_thread = ready_queue.front();
    running_thread->run_thread();
    ready_queue.pop_front();
}

/**
* Function that the timer calls to it when it is over
*/ 
void timer_handler(int new_state) {
    timer.timer_setup(&timer_handler, SIGVTALRM, 0, 0, ITIMER_VIRTUAL,
                      nullptr);
    int ret_value = sigsetjmp(&(running_thread->get_env()), 1);
    if (ret_value != 0) {
        if (ret_value == LONG_JUMP_VAL) {
            return;
        }
        UTError(SYSTEM_ERROR, SIGSET_ERROR);
        end_program(1);
        // exit(1);
    }
    running_thread->stop_thread(READY);
    scheduler();
}

/**
* Changes the threads according to the states
*/
void scheduler() {
    update_sleeping_threads();
    if (running_thread->get_state() == READY) {
        switch_threads(READY);
    } else if (running_thread->get_state() == BLOCKED) {
        switch_threads(BLOCKED);
    } else if (running_thread->get_state() == TERMINATED) {
        switch_threads(TERMINATED);
    }
    while (!terminated.empty()) {
        delete terminated[terminated.size() - 1];
        terminated.pop_back();
    }

    _ran_quantum++;
    timer.timer_setup(&timer_handler, SIGVTALRM, 0, _quantum_usecs,
                      ITIMER_VIRTUAL, nullptr);
    siglongjmp(&(running_thread->get_env()), LONG_JUMP_VAL);
}

/**
* Starts the uthreads library and run thread 0 as the calling program
*/
int uthread_init(int quantum_usecs) {
    if (quantum_usecs <= 0) {
        UTError(LIBRARY_ERROR, QUANTUM_TOO_SMALL);
        return FAILED;
    }

    // initiate the mean priority queue
    for (int i = 1; i < MAX_THREAD_NUM; ++i) {
        indexes.push(i);
    }

    // make sure the virtual alarm won't stop t0
    sigemptyset(&signal_set);
    sigaddset(&signal_set, SIGVTALRM);

    auto *first_thread = new (nothrow) UThread(nullptr);
    if (first_thread == nullptr) {
        UTError(SYSTEM_ERROR, ALLOCATION_ERROR);
        exit(1);
    }
    threads[0] = first_thread;
    running_thread = first_thread;

    _quantum_usecs = quantum_usecs;
    timer.timer_setup(&timer_handler, SIGVTALRM, TIME_DISABLE,
                      _quantum_usecs, ITIMER_VIRTUAL, nullptr);
    return SUCCESS;
}

/**
* Create another thread with the function of entry_point
*/
int uthread_spawn(thread_entry_point entry_point) {
    sigset_t actual;
    sigemptyset(&actual);
    sigprocmask(0, nullptr, &actual);
    sigprocmask(SIG_BLOCK, &signal_set, nullptr);
    if (indexes.empty()) {
        UTError(LIBRARY_ERROR, NO_THREADS_AVAILABLE);
        sigprocmask(SIG_SETMASK, &actual, nullptr);
        return FAILED;
    }
    if (entry_point == nullptr) {
        UTError(LIBRARY_ERROR, NO_ENTRYPOINT);
        sigprocmask(SIG_SETMASK, &actual, nullptr);
        return FAILED;
    }
    int index = (int)indexes.top();
    indexes.pop();
    UThread* nthread;
    try {
        nthread = new UThread(index, entry_point);
    } catch (std::bad_alloc&) {
        UTError(SYSTEM_ERROR, ALLOCATION_ERROR);
        end_program(1);
        // exit(1);
    }
    address_t sp = (address_t) nthread->get_sp() + STACK_SIZE - sizeof
            (address_t);
    address_t pc = (address_t) entry_point;
    sigsetjmp(&(nthread->get_env()), 1);
    ((&nthread->get_env())->__jmpbuf)[JB_SP] = translate_address(sp);
    ((&nthread->get_env())->__jmpbuf)[JB_PC] = translate_address(pc);
    sigemptyset(&((&nthread->get_env())->__saved_mask));
    threads[index] = nthread;
    ready_queue.push_back(nthread);
    sigprocmask(SIG_SETMASK, &actual, nullptr);
    return index;
}

/**
* Terminate the thread with the TID
*/
int uthread_terminate(int tid) {
    sigset_t actual;
    sigemptyset(&actual);
    sigprocmask(0, nullptr, &actual);
    sigprocmask(SIG_BLOCK, &signal_set, nullptr);


    if (tid == 0) {
        // exit(0);
        end_program(0);
    }
    if (tid > MAX_THREAD_NUM || tid < 0){
        UTError(LIBRARY_ERROR, TID_OUT_OF_RANGE_ERROR);
        sigprocmask(SIG_SETMASK, &actual, nullptr);
        return FAILED;

    }
    if ( threads[tid] == nullptr) {
        UTError(LIBRARY_ERROR, EMPTY_TID_ERROR);
        sigprocmask(SIG_SETMASK, &actual, nullptr);
        return FAILED;
    }

    if (running_thread->get_TID() != tid) {
        for (long unsigned int i = 0 ; i < blocked_threads.size() ; ++i) {
            if (blocked_threads[i]->get_TID() == tid) {
                blocked_threads.erase(blocked_threads.begin() + i);
            }
        } 
        for (long unsigned int  i = 0 ; i < ready_queue.size() ; ++i) {
            if (ready_queue[i]->get_TID() == tid) {
                ready_queue.erase(ready_queue.begin() + i);
            }
        }
        delete threads[tid];
        threads[tid] = nullptr;
        indexes.push(tid);
        sigprocmask(SIG_SETMASK, &actual, nullptr);
        return SUCCESS;
    }
    timer.timer_setup(&timer_handler, SIGVTALRM, TIME_DISABLE,
                      TIME_DISABLE, ITIMER_VIRTUAL, nullptr);
    threads[tid]->stop_thread(TERMINATED);
    scheduler();
    return FAILED;
}

/**
* Returns the total library ran number of quantums
*/ 
int uthread_get_total_quantums() {
    return _ran_quantum;
}

/**
* Returns the number of ran quantums of the thread with the TID
*/
int uthread_get_quantums(int tid) {
    sigset_t actual;
    sigemptyset(&actual);
    sigprocmask(0, nullptr, &actual);
    sigprocmask(SIG_BLOCK, &signal_set, nullptr);
    if (tid > MAX_THREAD_NUM || tid < 0){
        UTError(LIBRARY_ERROR, TID_OUT_OF_RANGE_ERROR);
        sigprocmask(SIG_SETMASK, &actual, nullptr);
        return FAILED;
    }
    if ( threads[tid] == nullptr) {
        UTError(LIBRARY_ERROR, EMPTY_TID_ERROR);
        sigprocmask(SIG_SETMASK, &actual, nullptr);
        return FAILED;
    }
    sigprocmask(SIG_SETMASK, &actual, nullptr);
    return threads[tid]->get_num_of_quantums();
}

/**
* Returns the running thread id
*/
int uthread_get_tid() {
    return running_thread->get_TID();
}

/**
* Removes the block status from a thread, and if it is not asleep returns it to ready queue
*/
int uthread_resume(int tid) {
    sigset_t actual;
    sigemptyset(&actual);
    sigprocmask(0, nullptr, &actual);
    sigprocmask(SIG_BLOCK, &signal_set, nullptr);
    if (tid > MAX_THREAD_NUM || tid < 0){
        UTError(LIBRARY_ERROR, TID_OUT_OF_RANGE_ERROR);
        sigprocmask(SIG_SETMASK, &actual, nullptr);
        return FAILED;
    }
    if ( threads[tid] == nullptr) {
        UTError(LIBRARY_ERROR, EMPTY_TID_ERROR);
        sigprocmask(SIG_SETMASK, &actual, nullptr);
        return FAILED;
    }
    UThread* blocked_thread = threads[tid];
    ProcessState state = blocked_thread->get_state();
    if (state == TERMINATED) {
        UTError(LIBRARY_ERROR, EMPTY_TID_ERROR);
        sigprocmask(SIG_SETMASK, &actual, nullptr);
        return FAILED;
    } else if (state == READY) {
        sigprocmask(SIG_SETMASK, &actual, nullptr);
        return SUCCESS;
    }
    // resume the thread
    if (state != RUNNING ) {
        blocked_thread->set_blocked(false);
        // change state to ready if needed
        if (threads[tid]->get_num_quantums() < 0){
            blocked_thread->set_state(READY);
            ready_queue.push_back(blocked_thread);
            for (long unsigned int  i = 0 ; i < blocked_threads.size() ; ++i) {
                if (blocked_threads[i]->get_TID() == tid) {
                    blocked_threads.erase(blocked_threads.begin() + i);
                    break;
                }
            }
        }
    }
    sigprocmask(SIG_SETMASK, &actual, nullptr);
    return SUCCESS;
}

/**
* Put the running thread to sleep num_quantums quantums
*/
int uthread_sleep(int num_quantums) {
    sigset_t actual;
    sigemptyset(&actual);
    sigprocmask(0, nullptr, &actual);
    sigprocmask(SIG_BLOCK, &signal_set, nullptr);

    if (num_quantums <= 0) {
        UTError(LIBRARY_ERROR, NEGATIVE_SLEEPING_TIME);
        sigprocmask(SIG_SETMASK, &actual, nullptr);
        return FAILED;
    }
    if (running_thread->get_TID() == 0) {
        UTError(LIBRARY_ERROR, SLEEP_THREAD0);
        sigprocmask(SIG_SETMASK, &actual, nullptr);
        return FAILED;
    }
    timer.timer_setup(&timer_handler, SIGVTALRM, 0, 0, ITIMER_VIRTUAL,
                      nullptr);
    int ret_value = sigsetjmp(&(running_thread->get_env()), SAVE_MASK);
    if (ret_value == LONG_JUMP_VAL) {
        sigprocmask(SIG_SETMASK, &actual, nullptr);
        return SUCCESS;
    }
    if (ret_value != 0) {
        UTError(SYSTEM_ERROR, SIGSET_ERROR);
        // exit(1);
        end_program(1);
    }
    running_thread->set_num_quantums(num_quantums);
    running_thread->stop_thread(BLOCKED);
    sigprocmask(SIG_SETMASK, &actual, nullptr);
    scheduler();
    return FAILED;
}

/**
* Blocks a thread with <TID> until it is resumed
*/
int uthread_block(int tid) {
    sigset_t actual;
    sigemptyset(&actual);
    sigprocmask(0, nullptr, &actual);
    sigprocmask(SIG_BLOCK, &signal_set, nullptr);
    if (tid > MAX_THREAD_NUM || tid < 0){
        UTError(LIBRARY_ERROR, TID_OUT_OF_RANGE_ERROR);
        sigprocmask(SIG_SETMASK, &actual, nullptr);
        return FAILED;

    }
    if ( threads[tid] == nullptr) {
        UTError(LIBRARY_ERROR, EMPTY_TID_ERROR);
        sigprocmask(SIG_SETMASK, &actual, nullptr);
        return FAILED;
    }
    if (tid == 0) {
        UTError(LIBRARY_ERROR, BLOCK_THREAD0);
        sigprocmask(SIG_SETMASK, &actual, nullptr);
        return FAILED;
    }
    if (tid != running_thread->get_TID()) {
        if (threads[tid]->get_state() == BLOCKED) {
            if (!(threads[tid]->get_blocked())) {
                threads[tid]->set_blocked(true);
            }
            sigprocmask(SIG_SETMASK, &actual, nullptr);
            return SUCCESS;
        }
        threads[tid]->set_blocked(true);
        threads[tid]->set_state(BLOCKED);
        int i = 0;
        for (UThread *ready_thread: ready_queue) {
            if (ready_thread->get_TID() == tid) {
                blocked_threads.push_back(ready_thread);
                ready_queue.erase(ready_queue.begin() + i);
                sigprocmask(SIG_SETMASK, &actual, nullptr);
                return SUCCESS;
            }
            ++i;
        }
        sigprocmask(SIG_SETMASK, &actual, nullptr);
        return SUCCESS;
    }
    timer.timer_setup(&timer_handler, SIGVTALRM, TIME_DISABLE,
                      TIME_DISABLE, ITIMER_VIRTUAL,
                      nullptr);
    int ret_value = sigsetjmp(&(running_thread->get_env()), SAVE_MASK);
    if (ret_value == LONG_JUMP_VAL) {
        sigprocmask(SIG_SETMASK, &actual, nullptr);
        return SUCCESS;
    }
    if (ret_value != 0) {
        UTError(SYSTEM_ERROR, SIGSET_ERROR);
        // exit(1);
        end_program(1);
    }
    running_thread->stop_thread(BLOCKED);
    running_thread->set_blocked(true);
    scheduler();
    return SUCCESS;
}
