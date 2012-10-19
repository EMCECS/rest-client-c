SRC=object.c rest_client.c
TESTSRC=seatest.c test_object.c test_rest_client.c test.c

OUTPUT_DIR=output
TEST_OUTPUT_DIR=testout

OBJ=$(addprefix $(OUTPUT_DIR)/,$(SRC:.c=.o))
TESTOBJ=$(addprefix $(TEST_OUTPUT_DIR)/,$(TESTSRC:.c=.o))

TESTEXE=$(TEST_OUTPUT_DIR)/rest_client_test

UNAME := $(shell uname)

VERSION = 1
LIBNAME = librest.a
SONAME = $(OUTPUT_DIR)/librest.so
LIBDIR = -L/usr/local/lib
LIBS= -lssl -lcrypto -lcurl
CFLAGS = -Iinclude -Wall -fPIC -g -c -D_FILE_OFFSET_BITS=64 -pthreads
SOFLAGS = -shared -Wl,-soname,$(LIBNAME) -o $(LIBNAME)

TESTLINK = -L$(OUTPUT_DIR) -lrest

ifeq ($(UNAME), Darwin)
	SONAME = $(OUTPUT_DIR)/librest.dylib
	SOFLAGS = -dynamiclib -undefined suppress -flat_namespace -o $(LIBNAME)
endif 

all: prepare lib testharness

prepare:
	mkdir -p $(OUTPUT_DIR)
	mkdir -p $(TEST_OUTPUT_DIR)

lib: prepare $(OBJ)
	gcc $(SOFLAGS) $(OBJ)
	cd $(OUTPUT_DIR); ar rcs $(LIBNAME) *.o
	
$(OUTPUT_DIR)/%.o : src/%.c
	gcc -c $(CFLAGS) $< -o $@ 
	
$(TEST_OUTPUT_DIR)/%.o : test/%.c
	gcc -c $(CFLAGS) $< -o $@ 
	
testharness: lib $(TESTOBJ)
	gcc -pg -g -o $(TESTEXE) $(TESTLINK) $(LIBS) $(LIBDIR) $(TESTOBJ)

test: testharness
	./$(TESTEXE)
	
doc: 
	doxygen Doxyfile

clean:
	rm -f *.o
	rm -f *.so
	rm -f *.dylib
	rm -rf html
	rm -rf $(OUTPUT_DIR)
	rm -rf $(TEST_OUTPUT_DIR)