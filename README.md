# Linux Scheduling Policy Demonstration

### Description
Implement a program on Linux which lets a user run multiple threads with different scheduling policies and show the working status of each thread.

### Demo Result
```
$ sudo ./sched_demo -n 4 -t 0.5 -s NORMAL,FIFO,NORMAL,FIFO -p -1,10,-1,30
Thread 3 is running
Thread 3 is running
Thread 3 is running
Thread 1 is running
Thread 1 is running
Thread 1 is running
Thread 2 is running
Thread 0 is running
Thread 2 is running
Thread 0 is running
Thread 2 is running
Thread 0 is running
```

The meanings of command-line arguments are:
- `-n <num_threads>`: number of threads to run simultaneously
- `-t <time_wait>`: duration of "busy" period
- `-s <policies>`: scheduling policy for each thread, SCHED_FIFO or SCHED_NORMAL.
- `-p <priorities>`: real-time thread priority for real-time threads
