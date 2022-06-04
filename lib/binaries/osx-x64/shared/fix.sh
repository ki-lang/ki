
FILES="./*"
for f in $FILES
do
  echo "Fix $f"
  install_name_tool -id "@rpath/$f" "$f"
done
