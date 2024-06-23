.PHONY: all crc clean

all: clean crc

crc:
	nasm -g -f elf64 -w+all -w+error -o crc.o crc.asm
	ld --fatal-warnings -o crc crc.o

clean:
	rm -rf *.o