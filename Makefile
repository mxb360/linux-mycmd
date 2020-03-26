CCFLG = -g -Wall

all: myls


%.o: %.c 
	gcc -c $< -o $@ $(CCFLG)

myls: myls.o 
	gcc myls.o -o myls

clean:
	rm -rf *.o myls
