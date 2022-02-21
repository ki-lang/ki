
mkdir flame
sudo perf record -a -F 99 -g -p $(pgrep -x test)
sudo perf script | ~/FlameGraph/stackcollapse-perf.pl > flame/out.perf-folded
~/FlameGraph/flamegraph.pl flame/out.perf-folded > flame/perf.svg
