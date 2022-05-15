
UNAME=$(uname)

if [ "$UNAME" == "Linux" ] ; then
	echo "# Linux install"
	sudo cp ./ki /usr/bin/
elif [ "$UNAME" == "Darwin" ] ; then
	echo "# macOS install"
	sudo cp ./ki /usr/local/bin/
elif [ "$UNAME" == "MINGW" ] ; then
	echo "# MinGW install"
	install.bat
else
	echo "Install script currently doesnt support: $UNAME"
fi
