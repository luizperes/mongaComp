#!/bin/bash
FILES="test/checks/*.monga"
COUNT="0"
OK=0
Wrongs=0
for f in $FILES
do
	let COUNT=COUNT+1
	name=$(echo $f | cut -f 1 -d '.')
	echo "Testing $name"
	cat $f | ./comp -check > $f.output
	if [ ! -f $name.answer ]; then
    	echo "no answer associated"
    	continue
	fi
	if(cmp $f.output $name.answer) then
		rm $f.output
		echo "OK"
		let OK=OK+1
	else
		echo "Wrong"
		diff $f.output $name.answer
	fi
done
echo "$OK OK of $COUNT in total"
#cat test/file1.monga | ./comp | grep "tokens"