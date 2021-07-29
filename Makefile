make :
	make clean && make unixLs

unixLs: main.c
	gcc -o unixLs main.c validators.c

clean:
	rm -f unixLs