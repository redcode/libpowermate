# libpowermate Makefile
# Copyright(C) 2004, 2005 Manuel Sainz de Baranda y Goñi

# Directory
PREFIX=/usr/local
BIN_DIR=$(PREFIX)/bin
LIB_DIR=$(PREFIX)/lib


# Compiler
CC=gcc
LD=ld
INSTALL=install -c

# Project
NAME=powermate
VERSION=1.0.0
SYSTEM_VERSION=1

# Files
OBJECT=$(NAME).o
LIB=lib$(NAME).so
LIB_NAME=$(LIB).$(SYSTEM_VERSION)
TARGET_NAME=$(LIB).$(VERSION)
SOURCE_FILES=$(NAME).c

# FLags
CC_FLAGS=$(CFLAGS)
LD_FLAGS_SHARED=-shared -soname $(LIB_NAME) $(LDFLAGS)

all: shared

shared:
	$(CC) $(CC_FLAGS) -o $(OBJECT) -c $(SOURCE_FILES)
	$(LD) $(LD_FLAGS_SHARED) -o $(TARGET_NAME) $(OBJECT)

clean:
	rm -f $(OBJECT)
	rm -f $(TARGET_NAME)

install:
	$(INSTALL)

#check_host:
#	@if [ `hostname | awk -F. '{print $$1}'` != $(HOST) ]; then \
#	  echo "ERROR!! Makefile is configured for $(HOST)."; \
#	  exit 1;\
#	fi;
