TIMEFORMAT='%R'
N=10
min=10000
run.sh $1
for i in `seq $N`
do
    t=`(time run.sh $1) 2>&1`
    if (( $(echo "$t < $min" | bc) )) ; then
	min=$t
    fi
done
echo "$1 $min"
