CC := gcc
LD := gcc

TARGET := bkg

CFLAGS += -std=c99
CFLAGS += -Wall -Wpedantic -Wextra -Wformat=2
CFLAGS += -MMD
CFLAGS += -D_GNU_SOURCE
CFLAGS += -I.
CFLAGS += -ggdb

LDLIBS := -lcurl -lpng

OBJ := bkg.o buf.o uri.o img/png.o
DEP := $(OBJ:.o=.d)

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	$(RM) $(OBJ) $(DEP) $(TARGET)

-include $(DEPS)

$(TARGET): $(OBJ)
