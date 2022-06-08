#!/bin/bash

OPENSSL_VERSION="1.1.1o"

#curl -O https://www.openssl.org/source/openssl-$OPENSSL_VERSION.tar.gz
tar -xvzf openssl-$OPENSSL_VERSION.tar.gz
mv openssl-$OPENSSL_VERSION openssl_i386
tar -xvzf openssl-$OPENSSL_VERSION.tar.gz
mv openssl-$OPENSSL_VERSION openssl_x86_64
cd openssl_i386
./Configure darwin-i386-cc -shared
make
cd ../
cd openssl_x86_64
./Configure darwin64-x86_64-cc -shared
make
cd ../
