#!/bin/sh

die () {
        echo "$1;$2"
        exit 0
}

WARN_LIMIT="1GB"
CRIT_LIMIT="500MB"

while getopts "c:w:" OPTION
do
    case $OPTION in
        c)
            CRIT_LIMIT="$OPTARG"
        ;;
        w)
            WARN_LIMIT="$OPTARG"
        ;;
        *)
            echo "Unexpected option: $OPTION"
            exit 1
        ;;
    esac
done

warn_limit=$(/usr/bin/numfmt --from=iec --suffix=B "${WARN_LIMIT}" | grep -E --only-matching '[0-9]+')
crit_limit=$(/usr/bin/numfmt --from=iec --suffix=B "${CRIT_LIMIT}" | grep -E --only-matching '[0-9]+')

#                                vvv grep mysqld process VmRSS return value in kB vvv
mysqld_used_mem=$(/usr/bin/numfmt --from=iec --suffix=B "$(grep VmRSS "/proc/$(pidof mysqld)/status" | grep -E --only-matching '[0-9]+')KB")
mysqld_used_mem_human=$(/usr/bin/numfmt --to=iec --suffix=B "${mysqld_used_mem}")
mysqld_used_mem_num=$(echo "${mysqld_used_mem}" | grep -E --only-matching '[0-9]+')

memory_limit=$(grep memory_limit /etc/dbaas.conf | grep -E --only-matching '[0-9]+')
memory_limit_human=$(echo "${memory_limit}B" | /usr/bin/numfmt --to=iec --suffix=B)

# this is approx how much memory automatics and mdb utils can use
memory_left=$((memory_limit - mysqld_used_mem_num))

if [ "$memory_left" -le "$crit_limit" ]
then
    retcode=2
    message="$mysqld_used_mem_human memory used of $memory_limit_human limit."
elif [ "$memory_left" -le "$warn_limit" ]
then
    retcode=1
    message="$mysqld_used_mem_human memory used of $memory_limit_human limit."
else
    retcode=0
    message="OK"
fi

die $retcode "$message"
