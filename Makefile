CCFLG = -g -Wall

all: myls myrm


%.o: %.c 
	gcc -c $< -o $@ $(CCFLG)

myls: myls.o 
	gcc myls.o -o myls

myrm: myrm.o 
	gcc myrm.o -o myrm

clean:
	rm -rf *.o myls myrm
