#include "uthreads.h"
#include <setjmp.h>
#include "Itimer.cpp"

typedef enum ProcessState {
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
} ProcessState;

class UThread {
private:
    ProcessState _state;
    int _TID;
    char *_stack;
    int _ran_quantum = 0;
    int _num_quantums = -1;
    thread_entry_point _thread_func;
    __jmp_buf_tag _env;
    bool _is_blocked;


public:
    UThread(int TID, thread_entry_point func) {
        _state = READY;
        _TID = TID;
        _thread_func = func;
        _stack = new char[STACK_SIZE];
        _is_blocked = false;
    }

    UThread(thread_entry_point func) {
        _state = RUNNING; // called from uthread_init()
        _TID = 0;
        _thread_func = func;
        _stack = nullptr;
        _ran_quantum = 1;
        _is_blocked = false;
    }

    ~UThread() {
        if (_stack != nullptr) {
            if (_TID == 0) {
                return;
            }
            delete[] _stack;
        }
    }


    void set_blocked(bool isBlocked) {
        _is_blocked = isBlocked;
    }


    bool get_blocked() const {
        return _is_blocked;
    }


    int get_TID() {
        return _TID;
    }

    ProcessState get_state() {
        return _state;
    }

    void set_state(ProcessState state) {
        _state = state;
    }

    void run_thread() {
        this->_state = RUNNING;
        this->_ran_quantum += 1;
        // start timer
    }

    int get_num_of_quantums() const {
        return _ran_quantum;
    }

    void stop_thread(ProcessState state) {
        set_state(state);
    }

    __jmp_buf_tag &get_env() {
        return _env;
    }

    char *get_sp() {
        return _stack;
    }

    int get_num_quantums() const {
        return _num_quantums;
    }

    void set_num_quantums(int num_quantums) {
        _num_quantums = num_quantums;
    }
};




