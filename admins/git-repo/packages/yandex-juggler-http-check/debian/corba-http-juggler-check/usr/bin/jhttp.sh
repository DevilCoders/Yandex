#!/bin/bash

# include parser
. /usr/lib/shflags/src/shflags

# define arguments
DEFINE_string 'host' 'localhost' 'hostname to check one with curl. For example - afisha.yandex.ru' 'n'
DEFINE_string 'port' '80' 'port to check' 'p'
DEFINE_string 'ischeck' '' 'script to run before check. If \$? != 0, then http check will say \"OK\"' 'i'
DEFINE_string 'ischeckargs' '' 'arguments for ischeck script' 'a'
DEFINE_string 'timeout' '3' 'Timeout for curl' 't'
DEFINE_string 'scheme' 'http' 'scheme for curl - http/https' 's'
DEFINE_string 'url' '/' 'URL to ask with curl. For example - /ping.xml. Should start with /' 'u'
DEFINE_string 'answerformat' 'code: %{http_code}' 'answer format of curl. See man for "-w" option. Should be set in quotes, like -w "%{http_code}"' 'w'
DEFINE_string 'curl' 'curl' 'address to curl binary' 'c'
DEFINE_string 'server' 'localhost' 'Host to ask for. Normally it is localhost' 'r'
DEFINE_string 'expect' 'code: 200' 'what we are expect from curl as signal that all ok.' 'e'

# parse the command-line
FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"

# check, have we run http_check now.
if [ "${FLAGS_ischeck}" != "" ]; then
        if [ -x ${FLAGS_ischeck} ]; then
                ${FLAGS_ischeck} ${FLAGS_ischeckargs}
                if [ $? != 0 ]; then
                        echo "0;OK - ischeck script not 0"
                        exit 0
                fi
        fi
fi

# running http check.
answer=$(${FLAGS_curl} --max-time ${FLAGS_timeout} ${FLAGS_scheme}://${FLAGS_server}:${FLAGS_port}${FLAGS_url} -H "Host: ${FLAGS_host}" -w "${FLAGS_answerformat}" -o /dev/null 2>/dev/null)
if [[ $answer != ${FLAGS_expect} ]]
        then echo "2; $answer"
else
        echo "0; check ok"
fi
