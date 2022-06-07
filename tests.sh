
FILES="./tests/build/*.ki"
for f in $FILES
do
  echo "Fix $f"
  fn_ext="$(basename -- $f)"
  fn="${fn_ext%.*}"
  echo "Base $fn"
  ./ki build $f -o ./tests/out/test $*
  if [[ $? != 0 ]]; then
	echo "Building test failed: $f"
	exit 1
  fi
  ./tests/out/test
  if [[ $? != 0 ]]; then
	echo "Running test failed: $f"
	exit 1
  fi
done
