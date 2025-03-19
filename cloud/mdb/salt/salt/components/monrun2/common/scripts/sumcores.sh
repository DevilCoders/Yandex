#!/bin/sh -e

# A POSIX variable
OPTIND=1         # Reset in case getopts has been used previously in the shell.

# Initialize our own variables:
dir='/var/cores/'
crit=5 # critical threshold
warn=1 # warning threshold

while getopts "h?d:w:c:" opt; do
    case "$opt" in
    h|\?)
        printf '%s\n' "$(basename "$0") [-h] [-d DIR] [-c C] [-w W] -- show amount of files in cores dir"
        printf '\n'
        printf '%s\n' "where:"
        printf '%s\n' "-h  show this help text"
        printf '%s\n' "-d  use DIR as cores dir. defaults to /var/cores/"
        printf '%s\n' "-c  fire crit if number_of_cores >= C. Default C=5"
        printf '%s\n' "-w  fire warn if number_of_cores >= W. Default W=1"
        exit 0
        ;;
    d)  dir="$OPTARG"
        ;;
    c)  crit=$OPTARG
        ;;
    w)  warn=$OPTARG
        ;;
    esac
done

shift $((OPTIND-1))

[ "$1" = "--" ] && shift

core_pattern=$(sysctl -n 'kernel.core_pattern')

beginswith() { case $2 in $1*) true;; *) false;; esac; }
contains() { case $2 in *$1*) true;; *) false;; esac; }
if ! beginswith "$dir" "$core_pattern" && ! contains '/usr/sbin/portod' "$core_pattern"; then
    echo "1;sysctl core_pattern '$core_pattern' does not match cores dir '$dir'"
    exit 0
fi
if ! contains '%' "$core_pattern"; then
    echo "1;sysctl core_pattern '$core_pattern' points to single file"
    exit 0
fi

num_cores=$(ls -1 $dir | wc -l)

if [ $num_cores -ge $crit ]; then
    echo "2;$num_cores"
elif [ $num_cores -ge $warn ]; then
    echo "1;$num_cores"
elif [ $num_cores -eq 0 ]; then
    echo "0;$num_cores"
else
    echo "2;Number of cores '%num_cores' is weird"
fi

# End of file
