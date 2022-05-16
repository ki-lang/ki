
UNAME=$(uname)

if [ "$UNAME" == "Linux" ] ; then
	echo "# Linux install"
elif [ "$UNAME" == "Darwin" ] ; then
	echo "# macOS install"
else
	echo "Install script currently doesnt support: $UNAME"
	exit 1
fi

sudo mkdir -p /opt/ki
sudo cp ./ki /opt/ki/ki
if [ ! -f "/opt/ki/lib" ]; then
	sudo rm -rf /opt/ki/lib
fi
sudo cp -r ./lib /opt/ki/
sudo mkdir -p /usr/local/bin
sudo ln -s -f /opt/ki/ki /usr/local/bin/ki

echo "# Done"
