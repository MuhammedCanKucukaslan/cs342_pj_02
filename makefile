a: 
	gcc -Wall -gdwarf-4 -o  systemsim systemsim.c -lpthread -lrt -lm

all: a

tar:
	tar czf 21901779.tar.gz systemsim.c makefile readme.txt report.pdf -f

run:a
	#     // <ALG> <Q> <T1> <T2> <burst-dist> <burstlen> <min-burst> <max-burst> <p0> <p1> <p2> <pg> <MAXP> <ALLP> <OUTMODE>

	./systemsim FCFS INF 5 10 fixed 100 10 100 0.1 0.8 0.1 0.5 3 5 3
	#./systemsim FCFS INF 5 10 fixed 100 10 100 0.8 0.1 0.1 0.1 5 10 0 
	
r: a
	./systemsim FCFS INF 5 10 fixed 100 10 100 0.3 0.3 0.4 0.5 3 5 3
rr: a
	./systemsim RR 5 50 100 fixed 20 10 100 0.3 0.4 0.4 0.9 3 5 3
val: a
	valgrind --leak-check=full --show-leak-kinds=all --leak-resolution=high --track-origins=yes --vgdb=yes ./systemsim FCFS INF 5 10 fixed 100 10 100 0.5 0.25 0.25 0.5 3 5 3
hel: a
	valgrind --tool=helgrind ./systemsim FCFS INF 5 10 fixed 100 10 100 0.1 0.8 0.1 0.5 3 5 3
clean:
	rm -fr *~ systemsim 
