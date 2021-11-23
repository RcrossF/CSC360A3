.phony all:
all: diskinfo disklist diskget diskput

diskinfo: diskinfo.c util.o util.h
	gcc -Wall diskinfo.c util.o -o diskinfo -g

disklist: disklist.c util.o util.h
	gcc -Wall disklist.c util.o -o disklist -g

diskget: diskget.c util.o util.h
	gcc -Wall diskget.c util.o -o diskget -g

diskput: diskput.c util.o util.h
	gcc -Wall diskput.c util.o -o diskput -g

.PHONY clean:
clean:
	-rm -rf *.o diskget diskput diskinfo disklist
