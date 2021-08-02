make :
	make clean && make UnixLs

UnixLs: main.c
	gcc -o UnixLs main.c validators.c

clean:
	rm -f UnixLs