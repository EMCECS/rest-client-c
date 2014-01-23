#!/bin/sh

## 
## Sets up and generates all of the stuff that
## automake and libtool do for us.
##

echo "Setting up libtool"
libtoolize --force
echo "Setting up aclocal"
aclocal -I m4
echo "Generating configure"
autoconf
echo "Generating config.h.in"
autoheader
echo "Generating Makefile.in"
automake --add-missing

