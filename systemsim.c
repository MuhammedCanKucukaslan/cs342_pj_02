#include "shared.h"
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

// declare global variables
int allp_count = 0;
struct pcb_queue ready_queue = {NULL, NULL, 0};
int toBeRun_pid = -1;// must be safeguarded by cpu_mutex
int io1_wait_count = 0;
int io2_wait_count = 0;
int running_process_count = 0;

pthread_mutex_t pcb_queue_mutex;            // mutex for adding to or removing from ready_queue
pthread_mutex_t cpu_mutex;                  // mutex for running on the CPU
pthread_mutex_t io1_mutex;                  // mutex for running on device1
pthread_mutex_t io2_mutex;                  // mutex for running on device2
pthread_mutex_t running_process_count_mutex;// mutex for running_process_count variable
pthread_mutex_t wakeup_sched_mutex;         // mutex for waking up scheduler
pthread_mutex_t io1_wait_count_mutex;       // mutex for io1_wait_count variable
pthread_mutex_t io2_wait_count_mutex;       // mutex for io2_wait_count variable


pthread_cond_t cpu_cond_var;   // condition variable for processes waiting on CPU
pthread_cond_t wakeup_sched;   // condition variable for waking up scheduler
pthread_cond_t wakeup_allProcs;// condition variable for waking up all processes by broadcast
pthread_cond_t io1_cond;       // condition variable for processes waiting on device1
pthread_cond_t io2_cond;       // condition variable for processes waiting on device2

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
    printf("Algorithm: %s\n", tmp_alg);
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

    // check T1 more than 30 less than 100
    if (T1 < 30 || T1 > 100) {
        printf("T1 must be than 30 less than 100\n");
        exit(1);
    }
    // check T2 at least 100 and at most 300
    if (T2 < 100 || T2 > 300) {
        printf("T2 must be at least 100 and at most 300\n");
        exit(1);
    }
    // ps. We assume inputs are correct! https://moodle.bilkent.edu.tr/2021-2022-spring/mod/forum/discuss.php?d=4265

    // initialize mutexes
    pthread_mutex_init(&pcb_queue_mutex, NULL);
    pthread_mutex_init(&cpu_mutex, NULL);
    pthread_mutex_init(&io1_mutex, NULL);
    pthread_mutex_init(&io2_mutex, NULL);

    pthread_mutex_init(&running_process_count_mutex, NULL);
    pthread_mutex_init(&wakeup_sched_mutex, NULL);
    pthread_mutex_init(&io1_wait_count_mutex, NULL);
    pthread_mutex_init(&io2_wait_count_mutex, NULL);


    // initialize condition variables
    pthread_cond_init(&cpu_cond_var, NULL);
    pthread_cond_init(&wakeup_sched, NULL);
    pthread_cond_init(&wakeup_allProcs, NULL);
    pthread_cond_init(&io1_cond, NULL);
    pthread_cond_init(&io2_cond, NULL);


    // initialize generator thread
    pthread_t gen_thread;
    pthread_create(&gen_thread, NULL, p_gen, NULL);

    // initialize scheduler thread
    pthread_t sched_thread;
    pthread_create(&sched_thread, NULL, p_sched, NULL);

    printf("SİZE %d\n",sizeof(struct PCB));

    /* thread join */
    pthread_join(gen_thread, NULL);
    pthread_join(sched_thread, NULL);
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
    

    return 0;
}// END main function

void *p_gen(void *arg)
{
    if(outmode == 0)
        printf("Generator thread started\n");

    // create ready queue
    // create at most as much as necessary and as much as allowed
    // https://moodle.bilkent.edu.tr/2021-2022-spring/mod/forum/discuss.php?d=3967

    // get lock of running process count
    pthread_mutex_lock(&running_process_count_mutex);
    for (int i = 0; i < 10 && i < MAXP; i++) {
        pthread_t tid;
        allp_count++;
        running_process_count++;
        // allp_count is the so-called pid of the processes
        pthread_create(&tid, NULL, process, (void *) (long) allp_count);
    }
    // release lock of running process count
    pthread_mutex_unlock(&running_process_count_mutex);

    pthread_mutex_lock(&wakeup_sched_mutex);
    pthread_cond_signal(&wakeup_sched);
    pthread_mutex_unlock(&wakeup_sched_mutex);

    while (allp_count < ALLP) {
        pthread_t tid;
        // sleep for 5ms
        usleep(5000);

        pthread_mutex_lock(&running_process_count_mutex);
        if (running_process_count < MAXP && frandom() < pg) {
            allp_count++;
            if(outmode == 0)
                printf("Generator: %d process will be created\n", allp_count);
            pthread_create(&tid, NULL, process, (void *) (long) allp_count);
            if(outmode == 0)
                printf("Generator: %d process created\n", allp_count);
            running_process_count++;
        }
        pthread_mutex_unlock(&running_process_count_mutex);
    }
    if(outmode == 0)
        printf("Generator thread exited allp_count\n");

    // TODO replace with thread_join
    // It is guaranteed that all processes are created and included in running_process_count
    // So we can safely assume that all processes are finished if the running_process_count is 0
    while (running_process_count > 0) {
        usleep(1000);
    }
    // nevertheless, sleep for 5 ms
    usleep(5000);

    if(outmode == 0)
        printf("Generator thread finished\n");
    pthread_exit(NULL);
}

void *p_sched(void *arg)
{    
    if(outmode == 0)
        printf("SCHEDULER STARTED\n");
    pthread_mutex_lock(&wakeup_sched_mutex);
    pthread_cond_wait(&wakeup_sched, &wakeup_sched_mutex);
    pthread_mutex_unlock(&wakeup_sched_mutex);

    while ((allp_count < ALLP || running_process_count > 0) ) {
        pthread_mutex_lock(&wakeup_sched_mutex);
        pthread_cond_wait(&wakeup_sched, &wakeup_sched_mutex);

        // try lock cpu_mutex i.e. check if need to schedule
        if (pthread_mutex_trylock(&cpu_mutex)) {

            toBeRun_pid = mutex_queue_rm();
            if (toBeRun_pid != -1) {
                pthread_cond_broadcast(&wakeup_allProcs);
            }
            if(outmode == 0)
                printf("SCHEDULER: %d process will be scheduled\n", toBeRun_pid);
            // else  empty queue what to do?
            // do  nothing

            pthread_mutex_unlock(&cpu_mutex);
        }
        pthread_mutex_unlock(&wakeup_sched_mutex);
    }
    if(outmode == 0)
        printf("SCHEDULER FINISHED\n");
    pthread_exit(NULL);
}

void *process(void *arg)
{
    if(outmode == 0)
        printf("Process %d started\n", (int) arg);

    // todo implement process
    int pid = (int) arg;
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
            .start_time = 1,// todo
            .finish_time = 1,
            .total_execution_time = 0,
    };

    do {
        // state is changed to Ready
        my_pcb.state = READY;
        mutex_queue_add(my_pcb);
        // signal to the scheduler that a process is added to the ready queue
        pthread_mutex_lock(&wakeup_sched_mutex);
        pthread_cond_signal(&wakeup_sched);
        pthread_mutex_unlock(&wakeup_sched_mutex);


        pthread_mutex_lock(&cpu_mutex);
        while (toBeRun_pid != my_pcb.pid) {
            pthread_cond_wait(&wakeup_allProcs, &cpu_mutex);
        }

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
        usleep(sleep_time * 1000);
        // update remaining burst length
        my_pcb.remaining_burst_length = my_pcb.remaining_burst_length - sleep_time;

        // todo update pcb variables

        pthread_mutex_unlock(&cpu_mutex);

        // do io/term only RR is completely done with the burst or burst ended in other algorithm
        if (my_pcb.remaining_burst_length == 0) {
            // wake up sched, so it can do its job
            pthread_mutex_lock(&wakeup_sched_mutex);
            pthread_cond_signal(&wakeup_sched);
            pthread_mutex_unlock(&wakeup_sched_mutex);

            float rn = frandom();
            if (rn < p0) {
                my_pcb.state = TERMINATED;
                if(outmode == 0)
                    printf("Process %d terminated\n", my_pcb.pid);
            } 
            else if (rn < p0 + p1) {
                if(outmode == 0)
                    printf("Process %d is blocked in IO1\n", my_pcb.pid);
                my_pcb.state = WAITING;
                // todo rethink about the lock & cond mechanism
                pthread_mutex_lock(&io1_mutex);
                if (io1_wait_count != 0) {
                    io1_wait_count++;
                    pthread_cond_wait(&io1_cond, &io1_mutex);
                }
                // simulate the IO run
                usleep(T1 * 1000);// convert ms to us
                // TODO update PCB
                io1_wait_count--;
                pthread_cond_signal(&io1_cond);
                pthread_mutex_unlock(&io1_mutex);
            } else if (rn <= 1) {
                my_pcb.state = WAITING;
                pthread_mutex_lock(&io2_mutex);
                if (io2_wait_count != 0) {
                    io2_wait_count++;
                    pthread_cond_wait(&io2_cond, &io2_mutex);
                }
                // simulate the IO run
                usleep(T2 * 1000);// convert ms to us
                // TODO update PCB
                io2_wait_count--;
                pthread_cond_signal(&io2_cond);
                pthread_mutex_unlock(&io2_mutex);
            }
            if (my_pcb.state != TERMINATED) {
                my_pcb.next_burst_length = gen_burst_length();
                my_pcb.remaining_burst_length = my_pcb.next_burst_length;
                if(outmode == 0) 
                    printf("Process %d isnot terminated, burslength %d ms\n", my_pcb.pid, my_pcb.next_burst_length);
            }
        }
    } while (my_pcb.state != TERMINATED);

    pthread_mutex_lock(&running_process_count_mutex);
    running_process_count--;
    pthread_mutex_unlock(&running_process_count_mutex);

    if(outmode == 0)
        printf("Process %d finished\n", pid);
    pthread_exit(NULL);
}

int gen_burst_length()
{
    if (burst_dist == UNI) {
        return (int) (frandom() * (max_burst - min_burst)) + min_burst;
    } else if (burst_dist == EXP) {
        //  return (int) (frandom() * (max_burst - min_burst)) + min_burst;
    } else if (burst_dist == FIX) {
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
    struct pcb_node *node = malloc(sizeof(struct pcb_node));
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
    return (float) rand() / RAND_MAX;
}