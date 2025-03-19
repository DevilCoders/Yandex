#!/bin/bash

die () {
    echo "$1;$2"
    exit 0
}

prefix="/usr/bin/sudo -n"

ip4_list="${prefix} /sbin/iptables -t filter --list-rules INPUT -w"
ip6_list="${prefix} /sbin/ip6tables -t filter --list-rules INPUT -w"
ip4_blocked_rule="(10\.0\.0\.1/32)|(10\.0\.0\.0/8).+(REJECT|DROP)"
ip6_blocked_rule="(fdef::1|2a02:6b8::).+(REJECT|DROP)"
blocked=""
status=0

${ip4_list} | grep -E ${ip4_blocked_rule} >/dev/null && blocked="ipv4"
status=$((${status}+${PIPESTATUS[0]}))
${ip6_list} | grep -E ${ip6_blocked_rule} >/dev/null && blocked="${blocked} ipv6"
status=$((${status}+${PIPESTATUS[0]}))

if [ ${status} -gt 0 ]
then
    die 1 "error running iptables"
fi

if [ "x${blocked}" != "x" ]
then
    die 1 "${blocked} traffic is blocked"
else
    die 0 "OK"
fi
