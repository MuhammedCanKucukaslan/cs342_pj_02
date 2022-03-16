a: 
	gcc -Wall -gdwarf-4 -o  systemsim systemsim.c -lpthread -lrt

tar:
	tar czf 21901779.tar.gz systemsim.c makefile readme.txt report.pdf -f

run:
	./systemsim FCFS INF 50 100 fixed 20 10 100 0.1 0.6 0.7 0.5 30 200 1 
	

val: a
	valgrind --leak-check=full --show-leak-kinds=all --leak-resolution=high --track-origins=yes --vgdb=yes ./systemsim FCFS INF 50 100 fixed 20 10 100 0.1 0.6 0.7 0.5 30 200 1 
	
clean:
	rm -fr *~ systemsim 
