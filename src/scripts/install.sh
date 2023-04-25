#!/bin/sh

UNAME=$(uname)

if [ "$UNAME" = "Linux" ] ; then
	echo "# Linux install"
elif [ "$UNAME" = "Darwin" ] ; then
	echo "# macOS install"
else
	echo "Install script currently doesnt support: $UNAME"
	exit 1
fi

dir=$(dirname -- "$0";)
ki_dir="/opt/ki/__VERSION__"

sudo mkdir -p "$ki_dir"
sudo cp $dir/ki "$ki_dir/ki"
if [ ! -f "$ki_dir/lib" ]; then
	sudo rm -rf "$ki_dir/lib"
fi
sudo cp -r $dir/lib "$ki_dir/"
sudo mkdir -p /usr/local/bin
sudo ln -s -f "$ki_dir/ki" /usr/local/bin/ki

echo "# Done"
