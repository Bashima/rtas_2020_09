LIB = libzygarde

OBJECTS = \
	zygarde.o \
	timing.o \

DEPS = libdnn libfixed libio libmat libmspdriver

override SRC_ROOT = ../../src

override CFLAGS += \
	-I$(SRC_ROOT)/include \
	-I$(SRC_ROOT)/include/$(LIB)

include $(MAKER_ROOT)/Makefile.$(TOOLCHAIN)
