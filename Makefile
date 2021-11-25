.phony all:
all: diskinfo disklist diskget diskput

diskinfo: diskinfo.c util.c util.h
	gcc -Wall diskinfo.c util.c -o diskinfo -g

disklist: disklist.c util.c util.h
	gcc -Wall disklist.c util.c -o disklist -g

diskget: diskget.c util.c util.h
	gcc -Wall diskget.c util.c -o diskget -g

diskput: diskput.c util.c util.h
	gcc -Wall diskput.c util.c -o diskput -g

.PHONY clean:
clean:
	-rm -rf *.o diskget diskput diskinfo disklist
