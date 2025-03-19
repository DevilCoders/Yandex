#!/bin/bash

# include parser
. /usr/lib/shflags/src/shflags

# define arguments
DEFINE_string 'host' 'localhost' 'Server for check. Default - localhost' 's'
DEFINE_string 'port' '80' 'port to check.' 'p'
DEFINE_string 'timeout' '2' 'timeout for check.' 't'
DEFINE_boolean 'udp' false 'Use --udp/-u to check udp port' 'u'

# parse variables
FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"

# running code
if [[ ${FLAGS_udp} == 0 ]] 
	then udp="-u"
else udp=""
fi

nc -zw ${FLAGS_timeout} "${FLAGS_host}" ${FLAGS_port} $udp && echo "0;OK" || (echo "2; error - connection failed to ${FLAGS_host}:${FLAGS_port}"; exit 1)

