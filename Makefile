all:build install

build:
	cc pcmrip.c -lm -o pcmrip -Wall
	cc pcmplay.c -lportaudio -lrt -lm -lasound -pthread -o pcmplay -Wall
	cc -o pcmrec pcmrec.c -lportaudio -lpthread -Wall
	cc -o pcmstream pcmstream.c -lportaudio -lpthread -lasound -Wall
	cc pcmneg.c -o pcmneg

install:
	sudo cp pcmrip /usr/bin/
	sudo cp pcmplay /usr/bin/
	sudo cp pcmrec /usr/bin/
	sudo cp pcmneg /usr/bin/