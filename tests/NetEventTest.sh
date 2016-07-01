#!/bin/bash

TESTNAME="NetEventTest"

mkdir $TESTNAME
cd $TESTNAME
function quit {
	# Clean up files
	cd ..
	rm -r $TESTNAME

	exit $1
}

mpirun -np 2 ../pholdio --end=100 --io-files=1 --synch=2 --lookahead=.05 --nlp=32 --nkp=2 --io-store=1 > out1.txt

if [[ `ls pholdio_checkpoint.* | wc -l | tr -s ' '` -ne 4 ]]; then
	echo "Failure: Checkpoint files don't exist"
	exit 1
elif [[ `grep "Net Events" out1.txt | tr -s ' ' | cut -d' ' -f 4` -ne 6313 ]]; then
	echo "Failure: Incorrect number of Net Events on Run 1"
	quit 1
fi

mpirun -np 2 ../pholdio --end=100 --io-files=1 --synch=2 --lookahead=.05 --nlp=32 --nkp=2 --io-store=0 > out2.txt

if [[ `grep "Net Events" out2.txt | tr -s ' ' | cut -d' ' -f 4` -ne 6424 ]]; then
	echo "Failure: Incorrect number of Net Events on Run 2"
	quit 1
fi

mpirun -np 2 ../pholdio --end=200 --io-files=1 --synch=2 --lookahead=.05 --nlp=32 --nkp=2 --io-store=2 > out3.txt

if [[ `grep "Net Events" out3.txt | tr -s ' ' | cut -d' ' -f 4` -ne 12737 ]]; then
	echo "Failure: Incorrect number of Net Events on Run 3"
	quit 1
fi

echo "Success"
quit 0

