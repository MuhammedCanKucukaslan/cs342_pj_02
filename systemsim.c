#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>

#include "shared.c" 

int main(int argc, char **argv) {
    
    
    // READ THE arguments
    if (argc != 16) {
        printf("Usage: %s <message queue name> arg. count %s\n", argv[0], argc);
        exit(1);
    }
    // <ALG> <Q> <T1> <T2> <burst-dist> <burstlen> <min-burst> <max-burst> <p0> <p1> <p2> <pg> <MAXP> <ALLP> <OUTMODE>
    char* tmp_alg = argv[1];
    enum algorithm alg;
    if(strcmp(tmp_alg, "FCFS") == 0) {
        alg = FCFS;
    } else if(strcmp(tmp_alg, "SJF") == 0) {
        alg = SJF;
    } else if(strcmp(tmp_alg, "RR") == 0) {
        alg = RR;
    }
    else {
        printf("Invalid algorithm %s\n", tmp_alg);
        exit(1);
    }
    
    printf("Algorithm: %s\n", tmp_alg);

    int Q = -1;
    if (alg  == RR) {
        Q = atoi(argv[2]);
    }    

    int T1 = atoi(argv[3]);
    int T2 = atoi(argv[4]);

    char* tmp_burst_dist = argv[5];
    enum burst_distribution burst_dist;
    if(strcmp(tmp_burst_dist, "uniform") == 0) {
        burst_dist = UNI;
    } else if(strcmp(tmp_burst_dist, "exponential") == 0) {
        burst_dist = EXP;
    } else if(strcmp(tmp_burst_dist, "fixed") == 0) {
        burst_dist = FIX;
    }
    else {
        printf("Invalid burst distribution %s\n", tmp_burst_dist);
        exit(1);
    }

    int burstlen = atoi(argv[6]);
    int min_burst = atoi(argv[7]);
    int max_burst = atoi(argv[8]);
    double p0 = atof(argv[9]);
    double p1 = atof(argv[10]);
    double p2 = atof(argv[11]);
    double pg = atof(argv[12]);
    int MAXP = atoi(argv[13]);
    int ALLP = atoi(argv[14]);
    int outmode = atoi(argv[15]);

    // todo implement constraints
    // check T1 more than 30 less than 100
    if(T1 < 30 || T1 > 100) {
        printf("T1 must be than 30 less than 100\n");
        exit(1);
    }
    // check T2 at least 100 and at most 300
    if(T2 < 100 || T2 > 300) {
        printf("T2 must be at least 100 and at most 300\n");
        exit(1);
    }
    


} // END main function