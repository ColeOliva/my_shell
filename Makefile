myshell: myshell.o argparse.o builtin.o
	gcc -g -o myshell myshell.o argparse.o builtin.o

myshell.o: myshell.c argparse.h builtin.h
	gcc -g -c myshell.c

argparse.o: argparse.c argparse.h
	gcc -g -c argparse.c

builtin.o: builtin.c builtin.h
	gcc -g -c builtin.c

clean:
	rm -f myshell myshell.exe
	rm -f *.o *.bak *~*