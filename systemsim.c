#include "shared.h"
#include <math.h>
#include <mqueue.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

// declare arguments
enum algorithm alg;
int Q;
int T1;
int T2;
enum burst_distribution burst_dist;
int burstlen;
int min_burst;
int max_burst;
double p0;
double p1;
double p2;
double pg;
int MAXP;
int ALLP;
int outmode;
struct timeval sim_start_date;
struct timeval sim_end_date;

// declare global variables
int allp_count = 0;// safeguarded by running_process_count_mutex
struct pcb_queue ready_queue = {NULL, NULL, 0};
int toBeRun_pid = -1;// must be safeguarded by cpu_mutex
int io1_wait_count = 0;
int io2_wait_count = 0;
int running_process_count = 0;// safeguarded by running_process_count_mutex
int scheduler_started = -1;
struct PCB *pcb_array;
int cpu_available = 0;// 0 is not available, 1 is available

pthread_mutex_t pcb_queue_mutex;            // mutex for adding to or removing from ready_queue
pthread_mutex_t cpu_mutex;                  // mutex for running on the CPU
pthread_mutex_t io1_mutex;                  // mutex for running on device1
pthread_mutex_t io2_mutex;                  // mutex for running on device2
pthread_mutex_t running_process_count_mutex;// mutex for running_process_count variable
pthread_mutex_t wakeup_sched_mutex;         // mutex for waking up scheduler
pthread_mutex_t io1_wait_count_mutex;       // mutex for io1_wait_count variable
pthread_mutex_t io2_wait_count_mutex;       // mutex for io2_wait_count variable
pthread_mutex_t create_pgen_mutex;          // mutex for creating pgen and condition variable create_pgen_cond
pthread_mutex_t cpu_avail_mutex;            // mutex for creating pgen and condition variable create_pgen_cond


pthread_cond_t cpu_cond_var;   // condition variable for processes waiting on CPU
pthread_cond_t wakeup_sched;   // condition variable for waking up scheduler
pthread_cond_t wakeup_allProcs;// condition variable for waking up all processes by broadcast
pthread_cond_t io1_cond;       // condition variable for processes waiting on device1
pthread_cond_t io2_cond;       // condition variable for processes waiting on device2
pthread_cond_t
        running_process_count_cond;// the scheduler thread will wait for this condition variable to initiate scheduling
pthread_cond_t create_pgen_cond;   // the main thread will wait for this condition variable to create pgen,
pthread_cond_t sched_wake_barrier_cond;
// declare methods

void *p_gen(void *arg);
void *p_sched(void *arg);
void *process(void *arg);

int gen_burst_length();
int pcb_queue_dequeue();
void mutex_queue_add(struct PCB pcb);
int mutex_queue_rm();
void pcb_queue_insert(struct PCB pcb);

float frandom();
char *getStateName(enum state s);


void updateTime(struct timeval *ct_tv, long *elapsed_time);

int main(int argc, char **argv)
{

    // READ THE arguments
    if (argc != 16) {
        printf("Usage: %s <message queue name> arg. count %d\n", argv[0], argc);
        exit(1);
    }
    // <ALG> <Q> <T1> <T2> <burst-dist> <burstlen> <min-burst> <max-burst> <p0> <p1> <p2> <pg> <MAXP> <ALLP> <OUTMODE>
    char *tmp_alg = argv[1];
    if (strcmp(tmp_alg, "FCFS") == 0) {
        alg = FCFS;
    } else if (strcmp(tmp_alg, "SJF") == 0) {
        alg = SJF;
    } else if (strcmp(tmp_alg, "RR") == 0) {
        alg = RR;
    } else {
        printf("Invalid algorithm %s\n", tmp_alg);
        exit(1);
    }
    //printf("Algorithm: %s\n", tmp_alg);
    Q = -1;
    if (alg == RR) {
        Q = atoi(argv[2]);
    }
    T1 = atoi(argv[3]);
    T2 = atoi(argv[4]);
    char *tmp_burst_dist = argv[5];

    if (strcmp(tmp_burst_dist, "uniform") == 0) {
        burst_dist = UNI;
    } else if (strcmp(tmp_burst_dist, "exponential") == 0) {
        burst_dist = EXP;
    } else if (strcmp(tmp_burst_dist, "fixed") == 0) {
        burst_dist = FIX;
    } else {
        printf("Invalid burst distribution %s\n", tmp_burst_dist);
        exit(1);
    }

    burstlen = atoi(argv[6]);
    min_burst = atoi(argv[7]);
    max_burst = atoi(argv[8]);
    p0 = atof(argv[9]);
    p1 = atof(argv[10]);
    p2 = atof(argv[11]);
    pg = atof(argv[12]);
    MAXP = atoi(argv[13]);
    ALLP = atoi(argv[14]);
    outmode = atoi(argv[15]);

    // ps. We assume inputs are correct! https://moodle.bilkent.edu.tr/2021-2022-spring/mod/forum/discuss.php?d=4265
    srand(time(NULL));


    // allocate array for the process PCBs
    pcb_array = (struct PCB *) malloc(ALLP * sizeof(struct PCB));

    // initialize mutexes
    pthread_mutex_init(&pcb_queue_mutex, NULL);
    pthread_mutex_init(&cpu_mutex, NULL);
    pthread_mutex_init(&io1_mutex, NULL);
    pthread_mutex_init(&io2_mutex, NULL);

    pthread_mutex_init(&running_process_count_mutex, NULL);
    pthread_mutex_init(&wakeup_sched_mutex, NULL);
    pthread_mutex_init(&io1_wait_count_mutex, NULL);
    pthread_mutex_init(&io2_wait_count_mutex, NULL);
    pthread_mutex_init(&cpu_avail_mutex, NULL);


    // initialize condition variables
    pthread_cond_init(&cpu_cond_var, NULL);
    pthread_cond_init(&wakeup_sched, NULL);
    pthread_cond_init(&wakeup_allProcs, NULL);
    pthread_cond_init(&io1_cond, NULL);
    pthread_cond_init(&io2_cond, NULL);
    pthread_cond_init(&running_process_count_cond, NULL);
    pthread_cond_init(&sched_wake_barrier_cond, NULL);


    // initialize scheduler thread
    pthread_t sched_thread;
    pthread_create(&sched_thread, NULL, p_sched, NULL);

    // wait for scheduler thread to start

    // the program will be initiated succesfully regardless of whether the following code
    // executed before the sched_thread or after!
    // usleep(100*1000);
    pthread_mutex_lock(&create_pgen_mutex);
    while (scheduler_started < 0) {
        pthread_cond_wait(&create_pgen_cond, &create_pgen_mutex);
    }

    // initialize generator thread
    pthread_t gen_thread;
    pthread_create(&gen_thread, NULL, p_gen, NULL);
    // unlock
    pthread_mutex_unlock(&create_pgen_mutex);


    /* thread join */
    pthread_join(gen_thread, NULL);
    pthread_detach(sched_thread);
    pthread_cancel(sched_thread);

    //pthread_join(gen_thread, NULL);
    //pthread_cancel(sched_thread);


    // destroy them (mutexes) all!
    pthread_mutex_destroy(&pcb_queue_mutex);
    pthread_mutex_destroy(&cpu_mutex);
    pthread_mutex_destroy(&io1_mutex);
    pthread_mutex_destroy(&io2_mutex);
    pthread_mutex_destroy(&running_process_count_mutex);
    pthread_mutex_destroy(&wakeup_sched_mutex);
    pthread_mutex_destroy(&io1_wait_count_mutex);
    pthread_mutex_destroy(&io2_wait_count_mutex);

    // destroy them (condition variables) all!
    pthread_cond_destroy(&cpu_cond_var);
    pthread_cond_destroy(&wakeup_sched);
    pthread_cond_destroy(&wakeup_allProcs);
    pthread_cond_destroy(&io1_cond);
    pthread_cond_destroy(&io2_cond);
    pthread_cond_destroy(&running_process_count_cond);

    struct PCB cur_pcb;
    printf("%12s, %12s, %12s, %12s, %12s, %12s, %12s, %12s, %12s\n", "pid", "arv", "dept", "cpu", "waitr", "turna",
           "n-bursts", "n-d1", "n-d2");
    for (int i = 0; i < ALLP; i++) {
        cur_pcb = pcb_array[i];
        printf("%12d, %12d, %12d, %12d, %12d, %12d, %12d, %12d, %12d\n", cur_pcb.pid, cur_pcb.start_time,
               cur_pcb.finish_time, cur_pcb.total_execution_time, cur_pcb.time_in_ready_list,
               cur_pcb.finish_time - cur_pcb.start_time, cur_pcb.num_bursts, cur_pcb.IO_device1, cur_pcb.IO_device2);
    }
    free(pcb_array);
    return 0;
}// END main function

void *p_gen(void *arg)
{
    struct timeval current_time;
    pthread_t *created_threads = (pthread_t *) malloc(ALLP * sizeof(pthread_t));
    int thread_count = 0;
    if (outmode == 3)
        printf("Generator thread started.\n");

    // create ready queue
    // create at most as much as necessary and as much as allowed
    // https://moodle.bilkent.edu.tr/2021-2022-spring/mod/forum/discuss.php?d=3967

    // get lock of running process count
    pthread_mutex_lock(&running_process_count_mutex);
    for (int i = 0; i < 10 && i < MAXP; i++) {
        pthread_t tid;
        allp_count++;
        running_process_count++;
        long i = allp_count;
        // allp_count is the so-called pid of the processes
        pthread_create(&tid, NULL, process, (void *) i);
        created_threads[thread_count] = tid;
        thread_count++;
    }
    // release lock of running process count

    if (outmode == 3)
        printf("pgen generated first min{10, %d} threads, running_process_count=%d.\n", MAXP, running_process_count);

    // signal to p_sched running_process_count is non zero
    pthread_cond_signal(&running_process_count_cond);
    pthread_mutex_unlock(&running_process_count_mutex);

    pthread_mutex_lock(&running_process_count_mutex);
    while (allp_count < ALLP) {
        pthread_mutex_unlock(&running_process_count_mutex);
        pthread_t tid;
        // sleep for 5ms
        usleep(5000);

        pthread_mutex_lock(&running_process_count_mutex);
        if (running_process_count < MAXP && frandom() < pg) {
            allp_count++;
            if (outmode == 3)
                printf("Generator: process %d  will be created\n", allp_count);
            pthread_create(&tid, NULL, process, (void *) (long) allp_count);
            running_process_count++;
            created_threads[thread_count] = tid;
            thread_count++;
            if (outmode == 3)
                printf("Generator: process %d created, running_process_count: %d \n", allp_count,
                       running_process_count);
        }
    }
    pthread_mutex_unlock(&running_process_count_mutex);
    if (outmode == 0)
        printf("Generator thread exited allp_count\n");

    // TODO replace with thread_join
    // It is guaranteed that all processes are created and included in running_process_count
    // So we can safely assume that all processes are finished if the running_process_count is 0
    /*
    while (running_process_count > 0) {
        usleep(1000);
        //printf("Generator waits running_process_count to reduce to zero from %d\n", running_process_count);
    }
    */
    for (int i = 0; i < thread_count; i++) {
        pthread_join(created_threads[i], NULL);
    }

    long elapsed_time = 0;
    updateTime(&current_time, &elapsed_time);
    if (outmode == 3)
        printf("\n%ld Generator THREAD EXITS:  allp_count: %d, running_process_count: %d\n\n", elapsed_time, allp_count,
               running_process_count);

    free(created_threads);
    pthread_exit(NULL);
}

void *p_sched(void *arg)
{
    //usleep(100*1000);
    gettimeofday(&sim_start_date, NULL);
    struct timeval current_time;
    long elapsed_time;
    if (outmode == 3)
        printf("0 Scheduler thread started\n");
    // get time of day and set it to the sim start time
    scheduler_started = 1;


    // obtain lock of queue and signal main thread to start p_gen
    // then release the queue once we know that running_process_count is not 0
    pthread_mutex_lock(&pcb_queue_mutex);

    // wait for generator to create at least one process
    pthread_mutex_lock(&running_process_count_mutex);

    pthread_mutex_lock(&create_pgen_mutex);
    pthread_cond_signal(&create_pgen_cond);
    pthread_mutex_unlock(&create_pgen_mutex);

    // It is guaranteed for allp_count to be positive but better to be safe than stay awake 4 A.M.
    while (running_process_count < 1 || allp_count < 1) {
        pthread_cond_wait(&running_process_count_cond, &running_process_count_mutex);
    }

    // release queue so that process can add themselves to the queue
    // since running_process_count
    int isFirstTime = 1;
    pthread_mutex_unlock(&pcb_queue_mutex);

    while (allp_count < ALLP || running_process_count > 0) {
        pthread_mutex_unlock(&running_process_count_mutex);


        pthread_mutex_lock(&cpu_mutex);
        pthread_mutex_lock(&pcb_queue_mutex);
        updateTime(&current_time, &elapsed_time);
        if (outmode == 3) {
            printf("%ld Scheduler begin while: allp_count: %d, running_process_count: %d, queue size: %d, io1 count: "
                   "%d, io2 count: %d  \n",
                   elapsed_time, allp_count, running_process_count, ready_queue.count, io1_wait_count, io2_wait_count);
        }
        while (ready_queue.count == 0 || toBeRun_pid != -1) {
            pthread_mutex_unlock(&pcb_queue_mutex);
            if (isFirstTime == 1) {
                isFirstTime = 0;
                // todo
                //pthread_mutex_unlock(&pcb_queue_mutex);
            }
            pthread_cond_wait(&wakeup_sched, &cpu_mutex);
            pthread_mutex_lock(&pcb_queue_mutex);
        }
        toBeRun_pid = pcb_queue_dequeue();
        pthread_mutex_unlock(&pcb_queue_mutex);
        pthread_cond_broadcast(&wakeup_allProcs);
        pthread_mutex_unlock(&cpu_mutex);
        /*

        updateTime(&current_time, &elapsed_time);
        if (outmode == 3) {
            printf("%ld Scheduler begin while: allp_count: %d, running_process_count: %d, queue size: %d, io1 count: "
                   "%d, io2 count: %d  \n",
                   elapsed_time, allp_count, running_process_count, ready_queue.count, io1_wait_count, io2_wait_count);
        }
        // should be sure if queue has all initial elements
        pthread_mutex_lock(&wakeup_sched_mutex);
        while (allp_count < MAXP || toBeRun_pid != -1) {
            updateTime(&current_time, &elapsed_time);
            if (outmode == 3) {
                printf("%ld Scheduler begin if: allp_count: %d, running_process_count: %d, queue size: %d, io1 count: "
                       "%d, io2 count: %d  \n",
                       elapsed_time, allp_count, running_process_count, ready_queue.count, io1_wait_count,
                       io2_wait_count);
            }

            pthread_mutex_lock(&cpu_avail_mutex);
            pthread_cond_signal(&sched_wake_barrier_cond);
            pthread_mutex_unlock(&cpu_avail_mutex);

            sleep(1);
            pthread_cond_wait(&wakeup_sched, &wakeup_sched_mutex);
        }
        updateTime(&current_time, &elapsed_time);
        if (outmode == 3) {
            printf("%ld Scheduler before if: allp_count: %d, running_process_count: %d, queue size: %d, io1 count: %d, "
                   "io2 count: %d , toBeRun_pid: %d \n",
                   elapsed_time, allp_count, running_process_count, ready_queue.count, io1_wait_count, io2_wait_count,
                   toBeRun_pid);
        }
        // try lock cpu_mutex i.e. check if need to schedule
        if (toBeRun_pid == -1 && pthread_mutex_trylock(&cpu_mutex) == 0) {
            updateTime(&current_time, &elapsed_time);
            if (outmode == 3) {
                printf("%ld Scheduler begin pthread_mutex_trylock: allp_count: %d, running_process_count: %d, queue "
                       "size: %d, io1 count: %d, io2 count: %d , toBeRun_pid: %d \n",
                       elapsed_time, allp_count, running_process_count, ready_queue.count, io1_wait_count,
                       io2_wait_count, toBeRun_pid);
            }
            toBeRun_pid = mutex_queue_rm();
            if (toBeRun_pid != -1) {
                updateTime(&current_time, &elapsed_time);
                if (outmode == 3) {
                    printf("%ld Scheduler send wake up signal: allp_count: %d, running_process_count: %d, queue size: "
                           "%d, io1 count: %d, io2 count: %d , toBeRun_pid: %d \n",
                           elapsed_time, allp_count, running_process_count, ready_queue.count, io1_wait_count,
                           io2_wait_count, toBeRun_pid);
                }
                pthread_cond_broadcast(&wakeup_allProcs);
                updateTime(&current_time, &elapsed_time);
                if (outmode == 3) {
                    printf("%ld Scheduler sent wake up signal: allp_count: %d, running_process_count: %d, queue size: "
                           "%d, io1 count: %d, io2 count: %d , toBeRun_pid: %d \n",
                           elapsed_time, allp_count, running_process_count, ready_queue.count, io1_wait_count,
                           io2_wait_count, toBeRun_pid);
                }
            }
            // find elapsed time
            updateTime(&current_time, &elapsed_time);
            if (outmode == 3) {
                if (toBeRun_pid != -1)
                    printf("%ld Scheduler: process %d removed from queue and will be scheduled\n", elapsed_time,
                           toBeRun_pid);
                else {
                    printf("%ld Scheduler: no process to be scheduled\n", elapsed_time);
                }
                printf("%ld Scheduler: allp_count: %d, running_process_count: %d\n", elapsed_time, allp_count,
                       running_process_count);
                if (ready_queue.count > 0) {
                    printf("Pid: %d\n", ready_queue.head->pcb.pid);
                }
            }

            // else  empty queue what to do?
            // do  nothing
            updateTime(&current_time, &elapsed_time);
            if (outmode == 3) {
                printf("%ld Scheduler empty queue: allp_count: %d, running_process_count: %d, queue size: %d, io1 "
                       "count: %d, io2 count: %d , toBeRun_pid: %d \n",
                       elapsed_time, allp_count, running_process_count, ready_queue.count, io1_wait_count,
                       io2_wait_count, toBeRun_pid);
            }

            pthread_mutex_unlock(&cpu_mutex);
        }
        updateTime(&current_time, &elapsed_time);
        if (outmode == 3) {
            printf("%ld Scheduler after if: allp_count: %d, running_process_count: %d, toberun: %d\n", elapsed_time,
                   allp_count, running_process_count, toBeRun_pid);
        }
        pthread_mutex_unlock(&wakeup_sched_mutex);
         */
        pthread_mutex_lock(&running_process_count_mutex);
    }// end of while
    pthread_mutex_unlock(&running_process_count_mutex);


    updateTime(&current_time, &elapsed_time);
    if (outmode == 3)
        printf("\n%ld SCHEDULER THREAD EXITS:  allp_count: %d, running_process_count: %d\n\n", elapsed_time, allp_count,
               running_process_count);
    pthread_exit(NULL);
}

void *process(void *arg)
{
    struct timeval current_time;
    long elapsed_time;

    updateTime(&current_time, &elapsed_time);
    if (outmode == 3)
        printf("%ld process %ld started\n", elapsed_time, (long) arg);

    int pid = (int) (long) arg;
    pthread_t tid = pthread_self();
    int gen_time = gen_burst_length();
    struct PCB my_pcb = {
            .pid = pid,
            .tid = tid,
            .state = READY,
            .next_burst_length = gen_time,
            .remaining_burst_length = gen_time,
            .num_bursts = 0,
            .time_in_ready_list = 0,
            .IO_device1 = 0,
            .IO_device2 = 0,
            .start_time = elapsed_time,// todo
            .finish_time = 1,
            .total_execution_time = 0,
    };
    long added_time = 0;
    do {
        // state is changed to Ready
        my_pcb.state = READY;
        mutex_queue_add(my_pcb);
        // get elapsed time
        updateTime(&current_time, &elapsed_time);
        added_time = elapsed_time;
        if (outmode == 3)
            printf("%ld Process %d added to ready queue.\n", elapsed_time, pid);

        // signal to the scheduler that a process is added to the ready queue
        /*
        pthread_mutex_lock(&cpu_avail_mutex);
        pthread_cond_wait(&sched_wake_barrier_cond, &cpu_avail_mutex);
        pthread_mutex_unlock(&cpu_avail_mutex);
        */
        pthread_mutex_lock(&cpu_mutex);
        pthread_cond_signal(&wakeup_sched);
        while (toBeRun_pid != my_pcb.pid) {
            pthread_cond_wait(&wakeup_allProcs, &cpu_mutex);
        }
        toBeRun_pid = -1;

        /* // we actually took the mutex, so we can assume that we are not in the ready list
            // but nevertheless we should consider the when we start simulation 
            updateTime(&current_time, &elapsed_time);
            my_pcb.time_in_ready_list = my_pcb.time_in_ready_list + elapsed_time - added_time;
        */
        // our turn to use cpu
        // state is RUNNING
        my_pcb.state = RUNNING;

        // todo simulate cpu usage
        // sleep according to RR algorithm
        // sleep for lesser of remaining_burst_length, Q
        int sleep_time = my_pcb.remaining_burst_length;
        if (alg == RR && Q < my_pcb.remaining_burst_length) {
            sleep_time = Q;
        }

        // find time elapsed since sim_start_date in ms
        updateTime(&current_time, &elapsed_time);
        if (outmode == 2) {
            // . If it is 2, the running thread will print out the current time (in ms
            // from the start of the simulation), its pid and its state to the screen.
            printf("%ld %d %s\n", elapsed_time, my_pcb.pid, getStateName(my_pcb.state));
        } else if (outmode == 3) {
            printf("%ld %d %s, burstlength = %d, sleep_time: %d\n", elapsed_time, my_pcb.pid,
                   getStateName(my_pcb.state), my_pcb.remaining_burst_length, sleep_time);
        }
        my_pcb.time_in_ready_list = my_pcb.time_in_ready_list + elapsed_time - added_time;

        usleep(sleep_time * 1000);
        // update remaining burst length
        my_pcb.remaining_burst_length = my_pcb.remaining_burst_length - sleep_time;
        // update total execution time
        my_pcb.total_execution_time = my_pcb.total_execution_time + sleep_time;
        // update number of bursts
        my_pcb.num_bursts++;


        // get elapsed time
        updateTime(&current_time, &elapsed_time);
        if (outmode == 3) {
            printf("%ld Process %d finished its burst.\n", elapsed_time, pid);
        }
        pthread_cond_signal(&wakeup_sched);
        pthread_mutex_unlock(&cpu_mutex);

        // do io/term only RR is completely done with the burst or burst ended in other algorithm
        if (my_pcb.remaining_burst_length == 0) {
            // wake up sched, so it can do its job
            pthread_mutex_lock(&wakeup_sched_mutex);
            pthread_cond_signal(&wakeup_sched);
            pthread_mutex_unlock(&wakeup_sched_mutex);

            float rn = frandom();

            // get elapse
            updateTime(&current_time, &elapsed_time);

            if (rn < p0) {
                my_pcb.state = TERMINATED;
                if (outmode == 3)
                    printf("%ld Process %d shall be terminated.\n", elapsed_time, pid);
            } else if (rn < p0 + p1) {
                my_pcb.state = WAITING;
                if (outmode == 3)
                    printf("%ld Process %d shall be put to waiting queue of DEVICE1, wait: %d.\n", elapsed_time, pid,
                           io1_wait_count);

                pthread_mutex_lock(&io1_wait_count_mutex);
                io1_wait_count++;
                if (io1_wait_count > 1) {
                    pthread_cond_wait(&io1_cond, &io1_wait_count_mutex);
                }
                pthread_mutex_unlock(&io1_wait_count_mutex);

                updateTime(&current_time, &elapsed_time);
                if (outmode == 2) {
                    //  print out the current time (ms), its pid, and the device that it is using.
                    printf("%ld %d USING DEVICE1\n", elapsed_time, my_pcb.pid);
                } else if (outmode == 3) {
                    printf("%ld %d USING DEVICE1, T1 = %d\n", elapsed_time, my_pcb.pid, T1);
                }

                // simulate the IO run
                usleep(T1 * 1000);// convert ms to us
                // update PCB
                my_pcb.IO_device1++;

                pthread_mutex_lock(&io1_wait_count_mutex);
                io1_wait_count--;
                pthread_cond_signal(&io1_cond);
                pthread_mutex_unlock(&io1_wait_count_mutex);
            } else if (rn <= 1) {
                my_pcb.state = WAITING;
                if (outmode == 3)
                    printf("%ld Process %d shall be put to waiting queue of DEVICE2, wait: %d.\n", elapsed_time, pid,
                           io2_wait_count);

                pthread_mutex_lock(&io2_wait_count_mutex);
                io2_wait_count++;
                if (io2_wait_count > 1) {
                    pthread_cond_wait(&io2_cond, &io2_wait_count_mutex);
                }
                pthread_mutex_unlock(&io2_wait_count_mutex);

                updateTime(&current_time, &elapsed_time);
                if (outmode == 2) {
                    //  print out the current time (ms), its pid, and the device that it is using.
                    printf("%ld %d USING DEVICE2\n", elapsed_time, my_pcb.pid);
                } else if (outmode == 3) {
                    printf("%ld %d USING DEVICE2, T2 = %d\n", elapsed_time, my_pcb.pid, T2);
                }

                // simulate the IO run
                usleep(T2 * 1000);// convert ms to us
                // update PCB
                my_pcb.IO_device2++;

                pthread_mutex_lock(&io2_wait_count_mutex);
                io2_wait_count--;
                pthread_cond_signal(&io2_cond);
                pthread_mutex_unlock(&io2_wait_count_mutex);
            }
            if (my_pcb.state != TERMINATED) {
                my_pcb.next_burst_length = gen_burst_length();
                my_pcb.remaining_burst_length = my_pcb.next_burst_length;
                updateTime(&current_time, &elapsed_time);
                if (outmode == 3)
                    printf("%ld Process %d is not terminated, burslength %d ms\n", elapsed_time, my_pcb.pid,
                           my_pcb.next_burst_length);
            }
        }
    } while (my_pcb.state != TERMINATED);

    pthread_mutex_lock(&running_process_count_mutex);
    running_process_count--;
    pthread_mutex_unlock(&running_process_count_mutex);

    updateTime(&current_time, &elapsed_time);
    if (outmode == 3)
        printf("%ld Process %d finished, running_process_count %d\n", elapsed_time, pid, running_process_count);
    // final calculation of PCB DATA
    my_pcb.finish_time = elapsed_time;
    pcb_array[pid - 1] = my_pcb;
    pthread_exit(NULL);
}

int gen_burst_length()
{
    float rn = frandom();
    if (burst_dist == UNI) {
        return (int) (rn * (max_burst - min_burst)) + min_burst;
    } else if (burst_dist == EXP) {
        /*
            If selection will be according to exponential distribution, an
            exponentially distributed random value will be selected. The l parameter of
            the exponential distribution will depend on the <burstlen> parameter value
            (in ms) entered at command line. No matter which way is used, the selected
            next CPU burst length value should be in range [<min-burst>, <max-burst>]
        */
        // recall that cdf p(x) = 1 - exp(-lambda * x) which generates a real number from interval [0,lambda]
        // first we need to generate a real number p from interval [0,1] whose x value we want to know
        // then we can use the inverse function of the exponential distribution to get the x value
        // lambda = burstlen
        float x = -log(1 - rn) / burstlen;// if rn = 0, x = 0, if rn = 1, x = inf
        // now we rescale the x value to the interval [min_burst, max_burst]
        return (int) (x * (max_burst - min_burst)) + min_burst;
        // so we need to convert it to [min_burst, max_burst]
        //float lambda = (float) (max_burst - min_burst) / (float) (1 - exp(-1 * lambda));
    } else// if (burst_dist == FIX)
    {
        return burstlen;
    }
}

void mutex_queue_add(struct PCB pcb)
{
    // lock mutex
    pthread_mutex_lock(&pcb_queue_mutex);
    // add pcb to queue
    pcb_queue_insert(pcb);
    // unlock mutex
    pthread_mutex_unlock(&pcb_queue_mutex);
};

int mutex_queue_rm()
{
    // lock mutex
    pthread_mutex_lock(&pcb_queue_mutex);
    // select next pcb
    int pid = pcb_queue_dequeue();
    // unlock mutex
    pthread_mutex_unlock(&pcb_queue_mutex);
    return pid;
};

void pcb_queue_insert(struct PCB pcb)
{
    enum algorithm ins_alg = alg;
    struct pcb_node *node = (struct pcb_node *) malloc(sizeof(struct pcb_node));
    node->pcb = pcb;
    node->next = NULL;
    if (ready_queue.count == 0) {
        ready_queue.head = node;
        ready_queue.tail = node;
    } else if (ins_alg == SJF) {
        struct pcb_node *curr = ready_queue.head;
        struct pcb_node *prev = NULL;
        while (curr != NULL && curr->pcb.next_burst_length < node->pcb.next_burst_length) {
            prev = curr;
            curr = curr->next;
        }
        if (prev == NULL) {
            node->next = ready_queue.head;
            ready_queue.head = node;
        } else {
            prev->next = node;
            node->next = curr;
        }
        if (curr == NULL)
            ready_queue.tail = node;
    } else {
        ready_queue.tail->next = node;
        ready_queue.tail = node;
    }
    ready_queue.count++;
}


int pcb_queue_dequeue()
{
    if (ready_queue.count == 0) {
        return -1;
    } else {
        struct pcb_node *node = ready_queue.head;
        ready_queue.head = node->next;
        ready_queue.count--;
        if (ready_queue.count == 0) {
            ready_queue.tail = NULL;
        }
        int id = node->pcb.pid;
        free(node);
        return id;
    }
}
float frandom()
{
    return (float) random() / RAND_MAX;
}

char *getStateName(enum state s)
{
    switch (s) {
        case READY:
            return "READY";
        case RUNNING:
            return "RUNNING";
        case WAITING:
            return "WAITING";
        case TERMINATED:
            return "TERMINATED";
        default:
            return "UNKNOWN";
    }
}


void updateTime(struct timeval *ct_tv, long *elapsed_time)
{
    gettimeofday(ct_tv, NULL);
    *elapsed_time = (ct_tv->tv_sec - sim_start_date.tv_sec) * 1000 + (ct_tv->tv_usec - sim_start_date.tv_usec) / 1000;
}