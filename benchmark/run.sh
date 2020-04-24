set -e
for f in `ls`
do
    type=`echo $f | sed 's/^.*\.\([^\.]*\)$/\1/'`
    if [ $type != "aig" ]; then
	continue
    fi
    $1 $f
done
#tuner.py --quiet --no-dups --test-limit 1000 "run.sh test_simplebdd" ../../../include/SimpleBddMan.hpp
