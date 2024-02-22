#define _XOPEN_SOURCE 600
#define SCHED_NORMAL 0
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <time.h>
#include <err.h>
#include <errno.h>

typedef struct {
    pthread_t thread_id;
    int thread_num;
    int s_policy;
    int s_priority;
    pthread_barrier_t *barrier;
    pthread_attr_t attr;
} thread_info_t;

int num_threads;
double time_wait;
pthread_barrier_t barrier;

void busy_wait(double second){
    struct timespec start_time, current_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // integer part
    end_time.tv_sec = start_time.tv_sec + (time_t)second;
    // floating point part
    end_time.tv_nsec = start_time.tv_nsec + (long)((second - (time_t)second) * 1e9);

    do{
        for(int i = 0; i < 100000; ++i) ;
        clock_gettime(CLOCK_MONOTONIC, &current_time);
    } while (current_time.tv_sec < end_time.tv_sec || (current_time.tv_sec == end_time.tv_sec && current_time.tv_nsec < end_time.tv_nsec));
}

void *thread_func(void *arg)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    thread_info_t* info = (thread_info_t*)arg;

    if(pthread_setaffinity_np(info->thread_id, sizeof(cpuset), &cpuset) != 0) {
        perror("pthread_setaffinity_np");
        exit(EXIT_FAILURE);
    }
    
    /* 1. Wait until all threads are ready */
    pthread_barrier_wait(info->barrier);

    /* 2. Do the task */ 
    for (int i = 0; i < 3; i++) {
        printf("Thread %d is running\n", (info->thread_num));
        /* Busy for <time_wait> seconds */
        busy_wait(time_wait);
    }

    /* 3. Exit the function  */
    pthread_exit(NULL);
}


int main(int argc, char *argv[]) {
    /* 1. Parse program arguments */
    int opt;
    int policy[5] = {0};
    int priority[5] = {0};
    char *token;

    while((opt = getopt(argc, argv, "n:t:s:p:")) != -1){
        switch (opt) {
            case 'n':
                num_threads = atoi(optarg);
                break;
            case 't':
                time_wait = atof(optarg);
                break;
            case 's':
                token = strtok(optarg, ",");
                // parse n policies
                for(int i = 0; i < num_threads; ++i){
                    if (token[0] == 'N'){
                        policy[i] = SCHED_NORMAL;
                    } else if (token[0] == 'F'){
                        policy[i] = SCHED_FIFO;
                    }
                    token = strtok(NULL, ",");
                }
                break;
            case 'p':
                token = strtok(optarg, ",");
                // parse n priorities
                for(int i = 0; i < num_threads; ++i){
                    priority[i] = atoi(token);
                    token = strtok(NULL, ",");
                }
                break;
            default:
                fprintf(stderr, "-n <num_threads> -t <time_wait> -s <policy> -p <priority>\n");
                exit(EXIT_FAILURE);
        }
    }
    
    
    /* 2. Create <num_threads> worker threads */
    thread_info_t thread_info[num_threads];

    if(pthread_barrier_init(&barrier, NULL, num_threads + 1) != 0){
        perror("pthread_barrier_init");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_threads; ++i){
        thread_info[i].thread_num = i;
        thread_info[i].s_policy = policy[i];
        thread_info[i].s_priority = priority[i];
        thread_info[i].barrier = &barrier;
    }


    /* 3. Set CPU affinity */

    for (int i = 0; i < num_threads; i++) {
        /* 4. Set the attributes to each thread */
        // initialize
        if(pthread_attr_init(&thread_info[i].attr) != 0){
            perror("pthread_attr_init");
            exit(EXIT_FAILURE);
        }

        // set inher
        if(pthread_attr_setinheritsched(&thread_info[i].attr, PTHREAD_EXPLICIT_SCHED) != 0){
            perror("pthread_setinheritsched");
            exit(EXIT_FAILURE);
        }

        // set policy
        if(pthread_attr_setschedpolicy(&thread_info[i].attr, thread_info[i].s_policy) != 0){
            perror("pthread_attr_setschedpolicy");
            exit(EXIT_FAILURE);
        }

        // set priority
        struct sched_param param;
        if(thread_info[i].s_priority != -1){
            param.sched_priority = thread_info[i].s_priority;
            if(pthread_attr_setschedparam(&thread_info[i].attr, &param) != 0){
                perror("pthread_attr_setschedparam");
                exit(EXIT_FAILURE);
            }
        } 

        // Create thread
        if(pthread_create(&thread_info[i].thread_id, &thread_info[i].attr, thread_func, &thread_info[i]) != 0){
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }

        // destroy attribute
        if(pthread_attr_destroy(&thread_info[i].attr) != 0){
            perror("pthread_attr_destroy");
            exit(EXIT_FAILURE);
        }
    }

    /* 5. Start all threads at once */
    pthread_barrier_wait(&barrier);


    /* 6. Wait for all threads to finish  */ 
    for(int i = 0; i < num_threads; ++i){
        if(pthread_join(thread_info[i].thread_id, NULL) != 0){
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }
    }

    pthread_barrier_destroy(&barrier);
    return 0;
}