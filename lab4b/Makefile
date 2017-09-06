#NAME: Kai Wong
#EMAIL: kaileymon@g.ucla.edu
#ID: 704451679

.SILENT:

default:
	gcc -Wall -lmraa -lm -g -o lab4b lab4b.c

check:
	./test.sh 704451679

clean:
	rm -rf lab4b *.txt *.tar.gz

dist:
	tar -czf lab4b-704451679.tar.gz lab4b.c Makefile README test.sh
