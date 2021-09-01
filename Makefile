main: nre.c
	gcc nre.c -o nre -lm -lpthread -Ofast

run:
	./nre
