CCFLG = -g -Wall -Werror

all: myls

myutils.o: myutils.c myutils.h
	gcc -c myutils.c -o myutils.o $(CCFLG)

%.o: %.c myutils.h
	gcc -c $< -o $@ $(CCFLG)

myls: myls.o myutils.o
	gcc myls.o myutils.o -o bin/myls

clean:
	rm -rf *.o
