CC := gcc
LD := gcc

TARGET := bkg

CFLAGS += -std=c99
CFLAGS += -Wall -Wpedantic -Wextra -Wformat=2
CFLAGS += -MMD
CFLAGS += -D_GNU_SOURCE
CFLAGS += -I.
CFLAGS += -Og -ggdb

LDLIBS := -lcurl -lImlib2 -lX11

OBJ := bkg.o buf.o uri.o img.o rpc/cmd.o rpc/parson.o rpc/rpc.o
DEP := $(OBJ:.o=.d)

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	$(RM) $(OBJ) $(DEP) $(TARGET)

-include $(DEP)

$(OBJ): Makefile

$(TARGET): $(OBJ)
