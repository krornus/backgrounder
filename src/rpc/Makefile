CC := gcc
LD := gcc

TARGET := rpc

CFLAGS += -std=c99
CFLAGS += -Wall -Wpedantic -Wextra -Wformat=2
CFLAGS += -MMD
CFLAGS += -D_GNU_SOURCE
CFLAGS += -I..

DEBUG := 1
ifeq ($(DEBUG),1)
CFLAGS += -Og -ggdb
endif

OBJ := rpc.o cmd.o parson.o
DEP := $(OBJ:.o=.d)

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	$(RM) $(OBJ) $(DEP) $(TARGET)

-include $(DEP)

$(OBJ): Makefile

$(TARGET): $(OBJ)
