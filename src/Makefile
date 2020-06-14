CC := gcc
LD := gcc

TARGET := bkg

CFLAGS += -std=c99
CFLAGS += -Wall -Wpedantic -Wextra -Wformat=2
CFLAGS += -MMD
CFLAGS += -D_GNU_SOURCE
CFLAGS += -I.
CFLAGS += -ggdb

LDLIBS := -lcurl -lpng -ljpeg

OBJ := bkg.o buf.o uri.o img.o img/png.o img/jpeg.o
DEP := $(OBJ:.o=.d)

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	$(RM) $(OBJ) $(DEP) $(TARGET)

-include $(DEPS)

$(TARGET): $(OBJ)
