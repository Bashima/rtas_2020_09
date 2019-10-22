OBJECTS = main.o bitcounter.o audio.o
EXEC = celebi

DEPS = libmspbuiltins libfixed libmat libdnn libalpaca libio libmsp libzygarde liblof librsa libmspdriver 

ifeq ($(BACKEND), tails)
DEPS += libdsp libmspdriver
endif

include tools/maker/Makefile
