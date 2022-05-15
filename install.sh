
UNAME=$(uname)

if [ "$UNAME" == "Linux" ] ; then
	echo "# Linux install"
	sudo cp ./ki /usr/bin/
elif [ "$UNAME" == "Darwin" ] ; then
	echo "# macOS install"
	sudo cp ./ki /usr/bin/
else
	echo "Install script currently doesnt support: $UNAME"
fi
