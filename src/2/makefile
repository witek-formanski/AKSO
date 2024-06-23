.PHONY: all mdiv clean

all: clean mdiv

mdiv:
	gcc -c -Wall -Wextra -std=c17 -O2 -o mdiv_example.o mdiv_example.c
	nasm -f elf64 -g -w+all -w+error -o mdiv.o mdiv.asm
	gcc -z noexecstack -o mdiv mdiv_example.o mdiv.o

clean:
	rm -rf *.o
