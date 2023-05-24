#!/usr/bin/env bash

DIST_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
TC_DIR="$DIST_DIR/toolchains"
LIB_DIR="$DIST_DIR/libraries"

LIN_X64="$TC_DIR/linux-x64"
LIN_ARM64="$TC_DIR/linux-arm64"
MAC_ANY="$TC_DIR/macos-11-3"
WIN_ANY="$TC_DIR/win-sdk"

LLVM_LIN_X64="$LIB_DIR/linux-llvm-15-x64"
LLVM_MAC_X64="$LIB_DIR/macos-llvm-15-x64"
LLVM_MAC_ARM64="$LIB_DIR/macos-llvm-15-arm64"
LLVM_WIN_X64="$LIB_DIR/win-llvm-15-x64"

##############
# Toolchains
##############

if [ ! -d "$LIN_X64" ]; then
	echo "Download linux-x64 toolchain"
	cd $TC_DIR
	wget "https://archive.ki-lang.dev/linux-x64.tar.gz"
	tar -xf "linux-x64.tar.gz" --checkpoint=.100
	rm "linux-x64.tar.gz"
fi

if [ ! -d "$LIN_ARM64" ]; then
	echo "Download linux-aarch64 toolchain"
	cd $TC_DIR
	wget "https://toolchains.bootlin.com/downloads/releases/toolchains/aarch64/tarballs/aarch64--glibc--stable-2022.08-1.tar.bz2"
	tar -xf "aarch64--glibc--stable-2022.08-1.tar.bz2" --checkpoint=.100
	rm "aarch64--glibc--stable-2022.08-1.tar.bz2"
	mv "aarch64--glibc--stable-2022.08-1" "linux-arm64" 
fi

if [ ! -d "$MAC_ANY" ]; then
	echo "Download macos toolchain"
	cd $TC_DIR
	wget "https://github.com/phracker/MacOSX-SDKs/releases/download/11.3/MacOSX11.3.sdk.tar.xz"
	tar -xf "MacOSX11.3.sdk.tar.xz" --checkpoint=.100
	rm "MacOSX11.3.sdk.tar.xz"
	mv "MacOSX11.3.sdk" "macos-11-3" 
fi

if [ ! -d "$WIN_ANY" ]; then
	echo "Download windows toolchain"
	cd $TC_DIR
	wget "https://archive.ki-lang.dev/win-sdk.tar.gz"
	tar -xf "win-sdk.tar.gz" --checkpoint=.100
	rm "win-sdk.tar.gz"
fi

#########
# LLVM
#########

if [ ! -d "$LLVM_LIN_X64" ]; then
	echo "Download LLVM linux x64"
	cd $LIB_DIR
	wget "https://archive.ki-lang.dev/linux-llvm-15-x64.tar.gz"
	tar -xf "linux-llvm-15-x64.tar.gz" --checkpoint=.100
	rm "linux-llvm-15-x64.tar.gz"
fi

if [ ! -d "$LLVM_MAC_X64" ]; then
	echo "Download LLVM macos x64"
	cd $LIB_DIR
	wget "https://archive.ki-lang.dev/macos-llvm-15-x64.tar.gz"
	tar -xf "macos-llvm-15-x64.tar.gz" --checkpoint=.100
	rm "macos-llvm-15-x64.tar.gz"
fi

if [ ! -d "$LLVM_MAC_ARM64" ]; then
	echo "Download LLVM macos arm64"
	cd $LIB_DIR
	wget "https://archive.ki-lang.dev/macos-llvm-15-arm64.tar.gz"
	tar -xf "macos-llvm-15-arm64.tar.gz" --checkpoint=.100
	rm "macos-llvm-15-arm64.tar.gz"
fi

if [ ! -d "$LLVM_WIN_X64" ]; then
	echo "Download LLVM windows x64"
	cd $LIB_DIR
	wget "https://archive.ki-lang.dev/win-llvm-15-x64.tar.gz"
	tar -xf "win-llvm-15-x64.tar.gz" --checkpoint=.100
	rm "win-llvm-15-x64.tar.gz"
fi
