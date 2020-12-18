build:
	touch history.log
	gcc -I -Wall  main.c -o shell -w

run:
	touch history.log
	gcc -I -Wall  main.c -o shell -w;
	clear;
	./shell;