#!/bin/bash
VER=$1
#URL="http://www.canonware.com/download/jemalloc/jemalloc-${VER}.tar.bz2"
URL="https://github.com/jemalloc/jemalloc/releases/download/${VER}/jemalloc-${VER}.tar.bz2"
echo "Downloading $URL"
wget $URL
#curl $URL > /tmp/jemalloc.tar.bz2
#tar xvjf /tmp/jemalloc.tar.bz2
tar -xvf jemalloc-${VER}.tar.bz2
rm -rf jemalloc
mv jemalloc-${VER} jemalloc
rm jemalloc-${VER}.tar.bz2*
echo "Use git status, add all files and commit changes."
