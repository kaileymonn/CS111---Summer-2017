#NAME: Kai Wong
#EMAIL: kaileymon@g.ucla.edu
#ID: 704451679

CC_DIR=/usr/local/cs/bin/
CFLAGS=-Wall -lrt -std=c99 -g -pthread

build: 	lab2_list.c
	$(CC_DIR)gcc $(CFLAGS) -o lab2_list SortedList.c lab2_list.c
clean:
	rm -rf lab2_list *.csv *.png lab2b-704451679.tar.gz *.prof  
tests:
	-./tests.sh
dist:
	make 
	make tests
	make graphs
	rm -rf lab2b-704451679.tar.gz
	tar -czf lab2b-704451679.tar.gz *.c *.h *.png *.gp *.csv Makefile README tests.sh profile.out
graphs:
	$(CC_DIR)gnuplot lab2_list.gp
profile:
	rm -rf *.prof *.out
	LD_PRELOAD=/usr/lib/libprofiler.so.0 CPUPROFILE=raw.prof ./lab2_list --threads=12 --iterations=1000 --sync=s
	google-pprof --text ./lab2_list raw.prof >> profile.out
	google-pprof --list=thread_routine ./lab2_list raw.prof >> profile.out


