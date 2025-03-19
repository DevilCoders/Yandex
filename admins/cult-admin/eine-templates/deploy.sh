#!/bin/bash
eines="eine"
profiles="$1"
scriptname=`basename $0`

eine_commit ()
{
	eine="$1" # eine name
	prof="$2" # target profile name
	file="$3" # profile source file

	echo "$prof commiting to ${eine}:"

	if [ -r "$file" ]; then
		eine -s ${eine}.yandex-team.ru profile add $prof rules $file;
	else
		echo "no such file $file, skipped"
	fi
}

if [[ -z "$profiles" ]]; then
	echo "Usage: $scriptname 'profile1 profile2 ... profileN'"
	exit 0
fi

for i in $eines; do 
	for j in $profiles; do
		if [[ "$i" = "eine141" ]]; then
			eine_commit "$i" "$j" "${j}-kvm.tpl"
		else
			eine_commit "$i" "$j" "${j}.tpl"
		fi
	done
done
