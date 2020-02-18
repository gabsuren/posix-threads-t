rel:
	gcc -Wall -s -O2 main.c ring_buffer.c -l pthread -o main
deb:
	gcc -Wall -g -DENABLE_TRACE main.c ring_buffer.c -l pthread -o main
tgz:
	tar czvf test.tar.gz main.c ring_buffer.c ring_buffer.h Makefile
