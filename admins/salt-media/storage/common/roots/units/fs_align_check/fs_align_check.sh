#!/bin/bash

parts=()

for d in /dev/sd*[a-z]; do
	for partnum in $(sudo sgdisk -p $d | sed -e '1,/Number *Start/ d' | awk '{print $1}'); do
		if sudo parted $d align-check opt $partnum | grep -q 'not aligned'; then
			parts=(${parts[*]} $d$partnum);
		fi
	done
done

if [ ${#parts} -eq 0 ]; then
	echo "0; All partitions aligned optimally"
else
	echo "2; Partitions ${parts[*]} are unaligned"
fi
