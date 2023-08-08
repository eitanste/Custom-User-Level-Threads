# Custom-User-Level-Threads     </a> <a href="https://www.cprogramming.com/" target="_blank" rel="noreferrer"> <img src="https://raw.githubusercontent.com/devicons/devicon/master/icons/c/c-original.svg" alt="c" width="40" height="40"/> </a> <a href="https://www.w3schools.com/cpp/" target="_blank" rel="noreferrer"> <img src="https://raw.githubusercontent.com/devicons/devicon/master/icons/cplusplus/cplusplus-original.svg" alt="cplusplus" width="40" height="40"/>

# Project Description: A Custom User-Level Thread Library

This project involves creating a custom user-level thread library, including a virtual timer class, error handling, and thread management. The primary goal is to provide a user-friendly interface for managing threads within a program. The library's main components include:

1. **Itimer.cpp:** This file implements a virtual timer class named `Itimer`, which allows users to configure and manage timers. The `Itimer` class provides functionalities to set timer handlers, update timer values, and start timers using the `setitimer` function. It encapsulates the interaction with the POSIX signals and timer mechanisms.

2. **uthreads.cpp:** This file contains the core thread management functionality. It includes the implementation of thread-related functions such as thread creation, termination, blocking, resuming, and sleeping. The library maintains a set of data structures to manage thread states, ready queues, blocked threads, and terminated threads. The file also handles signal handling for thread switching, ensuring that threads execute in a cooperative multitasking manner.

3. **UTError.cpp:** This file defines an error handling class `UTError` that assists in generating informative error messages for various scenarios encountered during the library's usage. It includes different error types such as allocation errors, invalid thread IDs, quantum time issues, and more.

4. **UThread.cpp:** This file implements the `UThread` class, representing a user-level thread. Each `UThread` object encapsulates its state, stack, quantum usage, and other relevant information. The class provides methods to control thread execution, such as starting, stopping, and blocking a thread.

5. **Makefile:** The provided Makefile is used to compile the library and create the static library file `libuthreads.a`. The Makefile also offers options for cleaning, dependency management, and creating a compressed archive.

## Usage:

To utilize this custom thread library in your program, follow these steps:

1. **Compilation:** Use the provided Makefile to compile the library and create the static library file `libuthreads.a`. Simply run `make` in your terminal.

2. **Linking:** In your program, include the necessary headers (`uthreads.h`, `UTError.h`, etc.) and link against `libuthreads.a` while compiling. For example:
   
   ```bash
   g++ -o my_program my_program.cpp -L. -luthreads -lpthread
   ```

3. **Initialization:** Initialize the thread library using `uthread_init(quantum_usecs)`, where `quantum_usecs` is the time slice for each thread in microseconds.

4. **Thread Management:** Use functions like `uthread_spawn`, `uthread_terminate`, `uthread_block`, `uthread_resume`, and `uthread_sleep` to manage threads within your program. These functions allow you to create, terminate, block, resume, and sleep threads.

5. **Quantum Information:** Obtain information about the number of total quantums using `uthread_get_total_quantums`, and retrieve the number of quantums a specific thread has run using `uthread_get_quantums`.

6. **Thread ID:** Identify the ID of the currently running thread using `uthread_get_tid`.

7. **Cleanup:** Once your program is finished, ensure you clean up the resources by using `uthread_terminate` on all threads (except the main thread) and calling `uthread_init` again to release allocated memory.

**Note:** The provided Makefile also includes a `tar` target to create a compressed archive of your project files for easy distribution.

## Building the Library

To build the library, simply run the `make` command. This will compile the library and generate the static library file `libuthreads.a`.

## Contact

For any questions or assistance, feel free to contact the library's developer at `Eitan.Stepanov@gmail.com`.

By following these steps, you can integrate the custom thread library into your project, enabling concurrent execution and better resource management in your programs.
