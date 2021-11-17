CC=g++
INCLUDE_DIR=
PACKAGES=`pkg-config --cflags --libs libbitcoin-system`

#flags
DEFAULT_FLAGS=-std=c++17 
CFLAGS=
ALL_FLAGS=$(DEFAULT_FLAGS) $(CFLAGS)

# TODO: review and improve this makefile

all: gen-address

gen-address:
	$(CC) -o gen-address generate-key-pair-and-address.cpp $(ALL_FLAGS) $(PACKAGES)

.PHONY: clean

clean:
	rm -f gen-address *.o $(INCLUDE_DIR)/*~ *~
