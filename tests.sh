
if [ $# -eq 1 ]; then
  echo "Run tests containing '$1'"
fi

FILES="./tests/build/*.ki"
for f in $FILES
do
  skip=0
  if [ $# -eq 1 ]; then
    skip=1
  fi
  if [ $skip -eq 1 ] && [[ "$f" =~ .*"$1".* ]]; then
    skip=0
  fi
  if [ $skip -eq 1 ]; then
    continue
  fi
  echo "Fix $f"
  fn_ext="$(basename -- $f)"
  fn="${fn_ext%.*}"
  echo "Base $fn"
  ./ki build $f -o ./tests/out/test
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
