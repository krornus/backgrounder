CC := gcc
LD := gcc

TARGET := bkg

CFLAGS += -std=c99
CFLAGS += -Wall -Wpedantic -Wextra -Wformat=2
CFLAGS += -MMD
CFLAGS += -D_GNU_SOURCE
CFLAGS += -I.
CFLAGS += -O3

LDLIBS := -lcurl -lImlib2 -lX11

LDFLAGS := -pie
