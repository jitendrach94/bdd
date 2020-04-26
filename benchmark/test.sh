T=360
#tuner.py --quiet --no-dups --test-limit $T "run.sh test_atbdd" ../../include/AtBddMan.hpp > log_atbdd.txt
#tuner.py --quiet --no-dups --test-limit $T "run.sh test_buddy" ../../include/BuddyMan.hpp > log_buddy.txt
#tuner.py --quiet --no-dups --test-limit $T "run.sh test_cacbdd" ../../include/CacBddMan.hpp > log_cacbdd.txt
#tuner.py --quiet --no-dups --test-limit $T "run.sh test_cudd" ../../include/CuddMan.hpp > log_cudd.txt
#tuner.py --quiet --no-dups --test-limit $T "run.sh test_simplebdd" ../../include/SimpleBddMan.hpp > log_simplebdd.txt

#measure.sh test_atbdd >> log.txt
#measure.sh test_buddy >> log.txt
#measure.sh test_cacbdd >> log.txt
#measure.sh test_cudd >> log.txt
#measure.sh test_simplebdd >> log.txt
#cd ../plasmall
measure.sh test_atbdd >> ../logdef.txt
measure.sh test_buddy >> ../logdef.txt
measure.sh test_cacbdd >> ../logdef.txt
measure.sh test_cudd >> ../logdef.txt
measure.sh test_simplebdd >> ../logdef.txt~
