#!/bin/bash

# include parser
. /usr/lib/shflags/src/shflags

# define arguments
DEFINE_string 'host' 'localhost' 'hostname to check one with curl. For example - afisha.yandex.ru' 'n'
DEFINE_string 'port' '80' 'port to check. Ignored, when --multiport used' 'p'
DEFINE_string 'ischeck' '' 'script to run before check. If \$? != 0, then http check will say \"OK\"' 'i'
DEFINE_string 'ischeckargs' '' 'arguments for ischeck script' 'a'
DEFINE_string 'timeout' '3' 'Timeout for curl' 't'
DEFINE_string 'scheme' 'http' 'scheme for curl - http/https' 's'
DEFINE_string 'url' '/' 'URL to ask with curl. For example - /ping.xml. Should start with /' 'u'
DEFINE_string 'answerformat' 'code: %{http_code}' 'answer format of curl. See man for "-w" option. Should be set in quotes, like -w "%{http_code}"' 'w'
DEFINE_string 'curl' 'curl' 'address to curl binary' 'c'
DEFINE_string 'server' 'localhost' 'Host to ask for. Normally it is localhost' 'r'
DEFINE_string 'expect' 'code: 200' 'what we are expect from curl as signal that all ok.' 'e'
DEFINE_string 'curloptions' '' 'additional options for curl' 'o'
DEFINE_string 'multiport' '' 'Allows to set multiport check. Fails on first failed port in this way. Ports have to be listed in this way: 80,81,8080,8081,etc. Warning, all other options (like --host) works even in multiport'
DEFINE_boolean 'antiflap' false 'antiflap protection - re-runs check 2nd time, if it is failed. Disabled by default, use -f/--antiflap to enable.' 'f'
DEFINE_boolean 'output' true 'use --nooutput, if you do not need server and url at error message'
DEFINE_boolean 'raw' false 'use --raw if you want to get content of page to $answer as one string'

# parse the command-line
FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"

# uncomment this to debug:
#set -x

getraw () {
answer=$(${FLAGS_curl} ${FLAGS_curloptions} --max-time ${FLAGS_timeout} ${FLAGS_scheme}://${FLAGS_server}:${FLAGS_port}${FLAGS_url} -H "Host: ${FLAGS_host}" 2>/dev/null | tr "\r\n" " ")
}

getformattedanswer () {
answer=$(${FLAGS_curl} ${FLAGS_curloptions} --max-time ${FLAGS_timeout} ${FLAGS_scheme}://${FLAGS_server}:${FLAGS_port}${FLAGS_url} -H "Host: ${FLAGS_host}" -w "${FLAGS_answerformat}" -o /dev/null 2>/dev/null)
}

getanswer () {
if [[ ${FLAGS_raw} != "0" ]] 
    then getformattedanswer 
    else getraw
fi
}

printerror () {
    if [[ ${FLAGS_output} == 0 ]]
	then echo "2; $answer (for ${FLAGS_scheme}://${FLAGS_host}:${FLAGS_port}${FLAGS_url})"
	else echo "2; $answer"
    fi
    return 1
}

printok () {
    echo "0; ok"
    return 0
}

checkanswer () {
    getanswer
    if [[ $answer != ${FLAGS_expect} ]] 
	then return 1
	else return 0
    fi
}

checkanswer_multiport () { 
for p in `echo ${FLAGS_multiport} | sed -r "s/,/ /g"`; do
    FLAGS_port=$p
    checkanswer;
#    echo $answer
    checkanswer || return 1;
done
}

checkanswer_antiflap () {
checkanswer && return 0 || checkanswer
}

runcheck () {
if [[ ${FLAGS_multiport} != "" ]]; then
    checkanswer_multiport
elif [[ ${FLAGS_antiflap} != 0 ]]
    then checkanswer
elif [[ ${FLAGS_antiflap} == 0 ]]
    then checkanswer_antiflap
fi
}

# check, have we run http_check now. This code suxx and have to be deleted.
if [ "${FLAGS_ischeck}" != "" ]; then
        if [ -x ${FLAGS_ischeck} ]; then
                ${FLAGS_ischeck} ${FLAGS_ischeckargs}
                if [ $? != 0 ]; then
                        echo "0;OK - ischeck script not 0"
                        exit 0
                fi
        fi
fi

# running code above:
runcheck && printok || printerror

