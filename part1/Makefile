all: empty part1

empty:
	gcc -o empty empty.c
	strace -o empty.trace ./empty

part1:
	gcc -o part1 part1.c
	strace -o part1.trace ./part1

clean:
	rm -f empty part1 
	# rm *.trace
	# Not sure if it should have *.trace
