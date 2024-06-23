# AKSO

## About

This repository contains my projects for the Computer Architecture and Operating Systenms course in my Bachelor's degree in Computer Science at the University of Warsaw.

## How to setup GDB to debug assembly?

In the **~/.bash_aliases** put this:
```
#!/bin/bash
alias gdb_asm="gdb -x ~/pathToRepository/AKSO/config/.gdbasm"
```
If the alias **gdb_asm** is still unrecognized, reboot or run:
```console
foo@AKSO:~$ source ~/.bash_aliases
```

## How to use GDB to debug assembly?

Run:
```console
foo@AKSO:~$ gdb_asm path/executable
```
The GDB window should pop out.

Here are the most useful GDB commands:
- b label - puts a breakpoint at the beginning of the given label
- b - puts a breakpoint at the current line
- b N - puts a breakpoint at line N
- d N - deletes breakpoint number N
- info break - lists breakpoints
- r - runs the program until a breakpoint or error
- r params - runs the program with given parameters
- c - continues running the program until the next breakpoint or error
- s - runs the next line of the program
- s N - Runs the next N lines of the program
- q - quits GDB

## Useful links

Here are the most useful reference sites for programming in NASM:
 - [Felix Cloutier (instructions reference)](http://www.felixcloutier.com/x86/)
 - [Linux System Call Table](https://blog.rchapman.org/posts/Linux_System_Call_Table_for_x86_64/)
 - [Understanding CRC](http://www.sunshine2k.de/articles/coding/crc/understanding_crc.html)
