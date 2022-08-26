.PHONY: all
all: main

main: main.c f.c
	gcc -m32 -no-pie -fno-pic -fomit-frame-pointer -mpreferred-stack-boundary=2 -O1 -o$@ $^

.PHONY: clean
clean:
	rm -f main
