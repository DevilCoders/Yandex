#!/bin/bash

# set defaults
CONF=/etc/monitoring/la_per_core.conf
force="0"  # force=1 to run check even for virtual hosts
w1=1.5; w5=1.2; w15=1
c1=1.5; c5=1.2; c15=1

[[ -r $CONF ]] && source $CONF

while getopts "w:c:" opt; do
    case "$opt" in
    h|\?)
        echo "Usage: $0 [-w L1,L5,L15] [-c L1,L5,L15]"
        exit 2
        ;;
    w)
        IFS=, read w1 w5 w15 <<< ${OPTARG}
        ;;
    c)
        IFS=, read c1 c5 c15 <<< ${OPTARG}
        ;;
    esac
done


load_average(){
    cc=$(grep -c processor /proc/cpuinfo)
    IFS=" " read la1 la5 la15 _ _ <<< $(cat /proc/loadavg)
    la1=$(bc -l <<< "$la1/$cc"); la5=$(bc -l <<< "$la5/$cc"); la15=$(bc -l <<< "$la15/$cc");

    if [[ $( bc <<< "($la1 > $c1) + ($la5 > $c5) + ($la15 > $c15)" ) -gt 0 ]]; then
      status=2
    elif [[ $( bc <<< "($la1 > $w1) + ($la5 > $w5) + ($la15 > $w15)" ) -gt 0 ]]; then
      status=1
    else
      status=0
    fi

    printf "%d;%.2f,%.2f,%.2f\n" $status $la1 $la5 $la15
    exit 0
}


. /usr/local/sbin/autodetect_environment
if [ "$force" == "1" ]; then
    load_average
elif [ "$(echo $HOSTNAME | egrep '^src-mskm9|^src-rtmp-mskm9|^src-mskstoredata|^src-rtmp-mskstoredata')" ]; then
    load_average
elif [ "$is_openvz_host" -eq 1 ] || [ "$is_lxc_host" -eq 1 ]; then
    echo "0;OK, virtual host, skip LA checking"
else
    load_average
fi

