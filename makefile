build:
	touch history.log
	gcc main.c -o shell -w

run:
	touch history.log
	gcc main.c -o shell -w;
	clear;
	./shell;