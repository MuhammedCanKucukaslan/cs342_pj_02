#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>

#include "shared.c"
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


void process();

int main(int argc, char **argv)
{
    // READ THE arguments
    if (argc != 16)
    {
        printf("Usage: %s <message queue name> arg. count %s\n", argv[0], argc);
        exit(1);
    }
    // <ALG> <Q> <T1> <T2> <burst-dist> <burstlen> <min-burst> <max-burst> <p0> <p1> <p2> <pg> <MAXP> <ALLP> <OUTMODE>
    char *tmp_alg = argv[1];

    if (strcmp(tmp_alg, "FCFS") == 0)
    {
        alg = FCFS;
    }
    else if (strcmp(tmp_alg, "SJF") == 0)
    {
        alg = SJF;
    }
    else if (strcmp(tmp_alg, "RR") == 0)
    {
        alg = RR;
    }
    else
    {
        printf("Invalid algorithm %s\n", tmp_alg);
        exit(1);
    }

    printf("Algorithm: %s\n", tmp_alg);

    Q = -1;
    if (alg == RR)
    {
        Q = atoi(argv[2]);
    }

    T1 = atoi(argv[3]);
    T2 = atoi(argv[4]);

    char *tmp_burst_dist = argv[5];
    if (strcmp(tmp_burst_dist, "uniform") == 0)
    {
        burst_dist = UNI;
    }
    else if (strcmp(tmp_burst_dist, "exponential") == 0)
    {
        burst_dist = EXP;
    }
    else if (strcmp(tmp_burst_dist, "fixed") == 0)
    {
        burst_dist = FIX;
    }
    else
    {
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

    // todo implement constraints
    // check T1 more than 30 less than 100
    if (T1 < 30 || T1 > 100)
    {
        printf("T1 must be than 30 less than 100\n");
        exit(1);
    }
    // check T2 at least 100 and at most 300
    if (T2 < 100 || T2 > 300)
    {
        printf("T2 must be at least 100 and at most 300\n");
        exit(1);
    }

} // END main function

struct pgen_arg
{
};

void *p_gen(void *arg)
{
    // create ready queue
    int maxp_count = 0;
    int allp_count = 0;
    for (int i = 0; i < 10; i++)
    {
        pthread_t tid;
        pthread_create(&tid, NULL, process, NULL);
        // TODO add tid to rdy queue
        maxp_count++;
        allp_count++;
    }

    while (allp_count <= ALLP)
    {
        pthread_t tid;
        // sleep for 5ms
        usleep(5000);
        float rn = ((float)rand()) / RAND_MAX;
        if (rn > pg)
        {
            pthread_create(&tid, NULL, process, NULL);
            // TODO add tid to rdy queue
            maxp_count++;
            allp_count++;
        }
    }
    // TODO wait for all processes to finish
    pthread_exit(NULL);
}

void *p_sched(void *arg)
{
    // create ready queue
    int maxp_count = 0;
    int allp_count = 0;
    for (int i = 0; i < 10; i++)
    {
        pthread_t tid;
        pthread_create(&tid, NULL, process, NULL);
        // TODO add tid to rdy queue
        maxp_count++;
        allp_count++;
    }

    while (allp_count <= ALLP)
    {
        pthread_t tid;
        // sleep for 5ms
        usleep(5000);
        float rn = ((float)rand()) / RAND_MAX;
        if (rn > pg)
        {
            pthread_create(&tid, NULL, process, NULL);
            // TODO add tid to rdy queue
            maxp_count++;
            allp_count++;
        }
    }
    // TODO wait for all processes to finish
    pthread_exit(NULL);
}

void process() {
    // todo implement process
     while (!terminated)
    {
            //  if (state == Ready)
            buf_add(buf, index, i);
            if (cpu)
            {
                    while (notmyturn)
                    {
                            pthread_cond_wait(cpu);
                    }
                    usleep(PCB.sleep_time);
                    // todo update PCB
                    determine next
            }
            else if(dev1) {
                    while (notmyturn)
                    {
                            pthread_cond_wait(dev1);
                    }
                    // todo use dev1
            }
            else if(dev2) {
                    while (notmyturn)
                    {
                            pthread_cond_wait(dev2);
                    }
                    // todo use dev2
            }
    }
    pthread_exit(NULL);
}