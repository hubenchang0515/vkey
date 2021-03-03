.PHONY: all clean

all: vkey

vkey: main.c
	gcc -ovkey main.c -O3 -W -Wall -Wextra 

clean:
	rm -f vkey
