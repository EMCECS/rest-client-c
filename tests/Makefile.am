TESTS = check_rest
check_PROGRAMS = check_rest
check_rest_SOURCES = seatest.c seatest.h test.c test.h test_object.c test_object.h test_rest_client.c test_rest_client.h
check_rest_LDADD = ../lib/librest.la $(CURL_LIBS)

LDADD = $(PTHREAD_LIBS)
AM_CFLAGS = $(PTHREAD_CFLAGS) -I$(srcdir)/../lib

