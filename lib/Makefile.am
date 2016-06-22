lib_LTLIBRARIES = librest.la
librest_la_SOURCES = object.c rest_client.c
librest_la_LDFLAGS = -version-info 0:0:0 $(CURL_LIBS)
include_HEADERS = maindoc.h object.h rest_client.h
pkgconfigdir = $(libdir)/pkgconfig
nodist_pkgconfig_DATA = rest-client-c.pc

LDADD = $(PTHREAD_LIBS)
AM_CFLAGS = $(PTHREAD_CFLAGS)


