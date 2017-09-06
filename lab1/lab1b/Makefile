#NAME: Kai Wong
#EMAIL: kaileymon@g.ucla.edu
#ID: 704451679

default: client server

client: lab1b-client.c
	gcc -Wall -lmcrypt -pthread -o lab1b-client -g lab1b-client.c
server:	lab1b-server.c
	gcc -Wall -lmcrypt -pthread -o lab1b-server -g lab1b-server.c

clean: 
	rm -rf lab1b-client lab1b-server

dist: 
	tar -cvzf lab1b-704451679.tar.gz lab1b-client.c lab1b-server.c Makefile my.key README
