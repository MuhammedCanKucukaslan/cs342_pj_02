a: 
	gcc -Wall -gdwarf-4 -o  systemsim systemsim.c -lpthread -lrt -lm

all: a

tar:
	tar czf 21901779.tar.gz systemsim.c makefile readme.txt shared.h report.pdf --ignore-failed-read

run:a
	#     // <ALG> <Q> <T1> <T2> <burst-dist> <burstlen> <min-burst> <max-burst> <p0> <p1> <p2> <pg> <MAXP> <ALLP> <OUTMODE>

	./systemsim FCFS INF 5 10 fixed 100 10 100 0.1 0.8 0.1 0.5 3 5 3
	#./systemsim FCFS INF 5 10 fixed 100 10 100 0.8 0.1 0.1 0.1 5 10 0 

tf: a
	./systemsim FCFS INF 10 20 fixed 50 10 100 0.7 0.2 0.1 0.6 5 300 0
	./systemsim FCFS INF 10 20 fixed 50 10 100 0.7 0.2 0.1 0.6 25 300 0
	./systemsim FCFS INF 10 20 exponential 5 10 100 0.7 0.2 0.1 0.6 5 300 0
	./systemsim FCFS INF 10 20 exponential 5 10 100 0.7 0.2 0.1 0.6 25 300 0
	./systemsim FCFS INF 10 20 uniform 5 10 100 0.7 0.2 0.1 0.6 5 300 0
	./systemsim FCFS INF 10 20 uniform 5 10 100 0.7 0.2 0.1 0.6 25 300 0

ts: a
	./systemsim SJF INF 10 20 fixed 50 10 100 0.7 0.2 0.1 0.6 5 300 0
	./systemsim SJF INF 10 20 fixed 50 10 100 0.7 0.2 0.1 0.6 25 300 0
	./systemsim SJF INF 10 20 exponential 5 10 100 0.7 0.2 0.1 0.6 5 300 0
	./systemsim SJF INF 10 20 exponential 5 10 100 0.7 0.2 0.1 0.6 25 300 0
	./systemsim SJF INF 10 20 uniform 5 10 100 0.7 0.2 0.1 0.6 5 300 0
	./systemsim SJF INF 10 20 uniform 5 10 100 0.7 0.2 0.1 0.6 25 300 0

tr5: a
	./systemsim RR 5 10 20 fixed 50 10 100 0.7 0.2 0.1 0.6 5 300 0
	./systemsim RR 5 10 20 fixed 50 10 100 0.7 0.2 0.1 0.6 25 300 0
	./systemsim RR 5 10 20 exponential 5 10 100 0.7 0.2 0.1 0.6 5 300 0
	./systemsim RR 5 10 20 exponential 5 10 100 0.7 0.2 0.1 0.6 25 300 0
	./systemsim RR 5 10 20 uniform 5 10 100 0.7 0.2 0.1 0.6 5 300 0
	./systemsim RR 5 10 20 uniform 5 10 100 0.7 0.2 0.1 0.6 25 300 0
tr10: a
	./systemsim RR 10 10 20 fixed 50 10 100 0.7 0.2 0.1 0.6 5 300 0
	./systemsim RR 10 10 20 fixed 50 10 100 0.7 0.2 0.1 0.6 25 300 0
	./systemsim RR 10 10 20 exponential 5 10 100 0.7 0.2 0.1 0.6 5 300 0
	./systemsim RR 10 10 20 exponential 5 10 100 0.7 0.2 0.1 0.6 25 300 0
	./systemsim RR 10 10 20 uniform 5 10 100 0.7 0.2 0.1 0.6 5 300 0
	./systemsim RR 10 10 20 uniform 5 10 100 0.7 0.2 0.1 0.6 25 300 0
tr50: a
	./systemsim RR 50 10 20 fixed 50 10 100 0.7 0.2 0.1 0.6 5 300 0
	./systemsim RR 50 10 20 fixed 50 10 100 0.7 0.2 0.1 0.6 25 300 0
	./systemsim RR 50 10 20 exponential 5 25 100 0.7 0.2 0.1 0.6 5 300 0
	./systemsim RR 50 10 20 exponential 5 25 100 0.7 0.2 0.1 0.6 25 300 0
	./systemsim RR 50 10 20 uniform 5 25 100 0.7 0.2 0.1 0.6 5 300 0
	./systemsim RR 50 10 20 uniform 5 25 100 0.7 0.2 0.1 0.6 25 300 0
r: a
	./systemsim FCFS INF 5 10 fixed 10 10 100 0.3 0.3 0.4 0.5 10 100 3
f: a
	./systemsim FCFS INF 5 10 fixed 10 10 100 0.3 0.3 0.4 0.5 10 100 3
rr: a
	./systemsim RR 10 10 20 fixed 30 10 100 0.3 0.4 0.4 0.9 10 1000 3
s:  a
	./systemsim SJF INF 10 20 fixed 20 10 100 0.3 0.4 0.4 0.9 13 100 3
val: a
	valgrind --leak-check=full --show-leak-kinds=all --leak-resolution=high --track-origins=yes --vgdb=yes ./systemsim FCFS INF 5 10 fixed 100 10 100 0.5 0.25 0.25 0.5 10 100 3
hel: a
	valgrind --tool=helgrind ./systemsim FCFS INF 5 10 fixed 100 10 100 0.1 0.8 0.1 0.5 3 5 3

hr: a
	valgrind --tool=helgrind ./systemsim FCFS INF 5 10 fixed 10 10 100 0.3 0.3 0.4 0.5 30 1000 2
hf: a
	valgrind --tool=helgrind ./systemsim FCFS INF 5 10 fixed 10 10 100 0.3 0.3 0.4 0.5 30 1000 2
hrr: a
	valgrind --tool=helgrind ./systemsim RR 5 50 100 fixed 20 10 100 0.3 0.4 0.4 0.9 13 1000 2
hs:  a
	valgrind --tool=helgrind ./systemsim SJF INF 50 100 fixed 20 10 100 0.3 0.4 0.4 0.9 23 1000 2

clean:
	rm -fr *~ systemsim 
