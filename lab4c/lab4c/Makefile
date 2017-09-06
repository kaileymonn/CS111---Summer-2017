#NAME: Kai Wong
#EMAIL: kaileymon@g.ucla.edu
#ID: 704451679

.SILENT:

default:
	gcc -Wall -lmraa -lm -g lab4c_tcp.c -o lab4c_tcp
	gcc -Wall -lmraa -lm -lssl -lcrypto -g lab4c_tls.c -o lab4c_tls

clean:
	rm -f lab4c_tcp lab4c_tls *.tar.gz *.txt 

dist:
	tar -czf lab4c-704451679.tar.gz lab4c_tcp.c lab4c_tls.c Makefile README 
