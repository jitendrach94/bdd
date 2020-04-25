set -e
for f in `ls`
do
    type=`echo $f | sed 's/^.*\.\([^\.]*\)$/\1/'`
    if [ $type != "aig" ]; then
	continue
    fi
    $1 $f
done
