CONTRIB_DIR = ..
HASHMAP_DIR = $(CONTRIB_DIR)/CHashMapViaLinkedList
GCOV_OUTPUT = *.gcda *.gcno *.gcov 
GCOV_CCFLAGS = -fprofile-arcs -ftest-coverage
SHELL  = /bin/bash
CC     = gcc
CCFLAGS = -g -O2 -Wall -Werror -Werror=return-type -Werror=uninitialized -Wcast-align -fno-omit-frame-pointer -fno-common -fsigned-char $(GCOV_CCFLAGS) -I$(HASHMAP_DIR) -Ilibuv/include -L.
LDFLAGS = -luv

ifeq ($(OS),Windows_NT)
    CCFLAGS += -D WIN32
    LDFLAGS += -lws2_32 -lpsapi -lIphlpapi
    ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
        CCFLAGS += -D AMD64
    endif
    ifeq ($(PROCESSOR_ARCHITECTURE),x86)
        CCFLAGS += -D IA32
    endif
else
    UNAME_S := $(shell uname -s)
    LDFLAGS += -ldl -lrt -Ipthread
    ifeq ($(UNAME_S),Linux)
        CCFLAGS += -D LINUX
    endif
    ifeq ($(UNAME_S),Darwin)
        CCFLAGS += -D OSX
    endif
    UNAME_P := $(shell uname -p)
    ifeq ($(UNAME_P),x86_64)
        CCFLAGS += -D AMD64
    endif
    ifneq ($(filter %86,$(UNAME_P)),)
        CCFLAGS += -D IA32
    endif
    ifneq ($(filter arm%,$(UNAME_P)),)
        CCFLAGS += -D ARM
    endif
endif


example: example.c $(HASHMAP_DIR)/linked_list_hashmap.c fff.c 
	$(CC) $(CCFLAGS) -o $@ $^ $(LDFLAGS)

all: example

chashmap:
	mkdir -p $(HASHMAP_DIR)/.git
	git --git-dir=$(HASHMAP_DIR)/.git init 
	pushd $(HASHMAP_DIR); git pull http://github.com/willemt/CHashMapViaLinkedList; popd

downloadcontrib: chashmap

clean:
	rm -f *.o tests $(GCOV_OUTPUT)
