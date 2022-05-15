
UNAME=$(uname)

if [ "$UNAME" == "Linux" ] ; then
	echo "# Linux install"
	sudo cp ./ki /usr/bin/
elif [ "$UNAME" == "Darwin" ] ; then
	echo "# macOS install"
	sudo mkdir -p /usr/local/bin
	sudo cp ./ki /usr/local/bin/
else
	echo "Install script currently doesnt support: $UNAME"
fi
