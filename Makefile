all: myls

myutils.o: myutils.c myutils.h
	gcc -c myutils.c -o myutils.o -Wall -Werror

%.o: %.c myutils.h
	gcc -c $< -o $@ -Wall -Werror

myls: myls.o myutils.o
	gcc myls.o myutils.o -o bin/myls

clean:
	rm -rf *.o
