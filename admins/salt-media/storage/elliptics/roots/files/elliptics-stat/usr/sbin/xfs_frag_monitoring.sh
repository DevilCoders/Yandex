#!/bin/bash


monrun(){ IFS=';'; echo "${level:-0};${msg[*]:-Ok}"; exit 0; }
setlevel(){ if [ ${level:-0} -lt $1 ]; then level=$1; fi; }

while getopts "w:c:" OPTION
do
    case $OPTION in
        w)
            WARN_LIMIT="$OPTARG"
        ;;
        c)
            CRIT_LIMIT="$OPTARG"
        ;;
    esac
done

warn_limit=${WARN_LIMIT:-30}
crit_limit=${CRIT_LIMIT:-50}

xfs_mount=`cat /proc/mounts  |grep xfs | awk '{print $1}'`

for point in $xfs_mount
do
	frag=`xfs_db -c frag -r $point | awk '{print $NF}' | sed 's/%//g'`
        if [[ $(echo "$frag >= $crit_limit" | bc) -eq 1 ]]
        then
         	setlevel 2;msg[m++]="$point: frag $frag%"
        elif [[ $(echo "$frag >= $warn_limit" | bc) -eq 1 ]]
	then
         	setlevel 1;msg[m++]="$point: frag $frag%"
        fi
done
        
monrun
