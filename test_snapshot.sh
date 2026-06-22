#!/bin/bash
# test script for snapshot with testadvanced6, which test a full run then 2 runs with a unteruption in between
make clean || true
make

#delete leftover from previous runs
rm -f snapshot.ijvmstate full.txt part1.txt part2.txt combined.txt

#run interpreter on mandelbread fully without interuption
./ijvm files/advanced/mandelbread.ijvm > full.txt
echo "exit code: $?"

./ijvm files/advanced/mandelbread.ijvm > part1.txt &
#save recent process id in variable PID
PID=$!

sleep 0.1

#send SIGINT signal to interrupt process
kill -SIGINT $PID 
wait $PID
echo "wait exit code: $?"

if [ ! -f snapshot.ijvmstate ]; then
    echo "ERROR: no snapshot created"
else
#Resume exectution on second part. The two parts get concatenated into one file combined.txt
#then we compare the full run to the two combined runs. If diff returns 0, that means there were no differences and the snapshot works correclty
    ./ijvm -r snapshot.ijvmstate > part2.txt
    cat part1.txt part2.txt > combined.txt
    diff full.txt combined.txt
    echo "diff result: $?"
fi
