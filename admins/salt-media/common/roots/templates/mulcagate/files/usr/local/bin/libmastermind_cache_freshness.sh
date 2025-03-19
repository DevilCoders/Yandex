#!/bin/sh

while getopts "m:" OPTION
do
    case $OPTION in
        m)
            MAX_STALE="$OPTARG"
        ;;
    esac
done

die() {
    echo "$1;$2"
    exit 0
}

max_stale=${MAX_STALE:-240}
files=(
/var/tmp/libmastermind.cache
)

for file in ${files[@]} ; do
        f=`ls -t $file*| head -n1`
        file="$f"
        actual_time=`date +%s`
        file_ctime=`stat -c%Z $file 2>/dev/null`
        if [[ $((actual_time-file_ctime)) -ge $max_stale ]];
            then die 2 "$file stale > $max_stale seconds"
        fi
done
die 0 "All files are up-to-date"
