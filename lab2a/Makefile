#NAME: Kai Wong
#EMAIL: kaileymon@g.ucla.edu
#ID: 704451679

build:	lab2_add.c lab2_list.c
	gcc -Wall -lrt -g -pthread -o lab2_add lab2_add.c
	gcc -Wall -lrt -std=c99 -g -pthread -o lab2_list SortedList.c lab2_list.c
clean:
	rm -rf lab2_add lab2_list *.csv *.png lab2a-704451679.tar.gz
tests:
	-./tests.sh
dist:
	make 
	make tests
	make graphs
	rm -rf lab2a-704451679.tar.gz
	tar -czf lab2a-704451679.tar.gz *.c *.h *.png *.gp *.csv Makefile README tests.sh
graphs:
	gnuplot lab2_add.gp
	gnuplot lab2_list.gp
