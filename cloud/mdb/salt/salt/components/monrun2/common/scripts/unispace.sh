#!/bin/sh

crit_flag=0
warn_flag=0
message=""

die () {
        echo "$1;$2"
        exit 0
}

while getopts "w:c:p:s" OPTION
do
    case $OPTION in
        w)
            WARN_LIMIT="$OPTARG"
        ;;
        c)
            CRIT_LIMIT="$OPTARG"
        ;;
        s)
            USE_SUDO="1"
        ;;
        p)
            EXACT_PATH="$OPTARG"
        ;;
    esac
done

warn_limit=${WARN_LIMIT:-90}
crit_limit=${CRIT_LIMIT:-97}

if [ ! -z "$USE_SUDO" ]
then
    cmd="sudo df"
else
    cmd="df"
fi

# Check for free space
out=$($cmd ${EXACT_PATH} -P -l -k -t ext2 -t ext3 -t ext4 -t xfs -t simfs 2>/dev/null | grep / | sort -u )
  
for i in $(echo "$out" | awk '{print $NF};')
do
    used_space=$(echo "$out" | grep " $i\$" | awk '{print $5};' | sed 's/%//g')
    if [ "$used_space" -ge "$crit_limit" ] 2>/dev/null
    then
        crit_flag=1
        message="$message$used_space% used on $i. "
    elif [ "$used_space" -ge "$warn_limit" ] 2>/dev/null
    then
        warn_flag=1
        message="$message$used_space% used on $i. "
    fi
done

if [ $crit_flag -ne 0 ]; then die 2 "$message"; fi
if [ $warn_flag -ne 0 ]; then die 1 "$message"; fi

# Check for free inodes

out=$($cmd ${EXACT_PATH} -i -P -l -k -t ext2 -t ext3 -t ext4 -t xfs -t simfs 2>/dev/null | grep / | sort -u )

for i in $(echo "$out" | awk '{print $NF};')
do
    used_inodes=$(echo "$out" | grep " $i\$" | awk '{print $5};' | sed 's/%//g')
    if [ "$used_inodes" -ge "$crit_limit" ]
    then
        crit_flag=1
        message="$message$used_inodes% inodes used on $i. "
    elif [ "$used_inodes" -ge "$warn_limit" ]
    then
        warn_flag=1
        message="$message$used_inodes% inodes used on $i. "
    fi
done

if [ $crit_flag -ne 0 ]; then die 2 "$message"; fi
if [ $warn_flag -ne 0 ]; then die 1 "$message"; fi

die 0 "OK"
