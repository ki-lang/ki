
FILES="./*"
for f in $FILES
do
  echo "Fix $f"
  install_name_tool -d "@rpath/$f" "$f"
done
