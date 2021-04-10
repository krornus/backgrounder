include config.mk

OBJ := bkg.o buf.o uri.o img.o
DEP := $(OBJ:.o=.d)

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	$(RM) $(OBJ) $(DEP) $(TARGET)

-include $(DEP)

$(OBJ): Makefile

$(TARGET): $(OBJ)
