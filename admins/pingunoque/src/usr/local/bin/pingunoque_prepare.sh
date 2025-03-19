#!/bin/bash

ipset=$(which ipset)
ip6tables=$(which ip6tables)
iptables=$(which iptables)
CFG=$(if [ -e /etc/yandex/pingunoque/config.yaml ]; then echo "/etc/yandex/pingunoque/config.yaml"; else echo "/etc/yandex/pingunoque/config-default.yaml";fi)
table_v4=$(fgrep 'set_v4:' ${CFG} | cut -f2 -d':' | tr -d ' ')
table_v6=$(fgrep 'set_v6:' ${CFG} | cut -f2 -d':' | tr -d ' ')
closed_ports=$(fgrep 'closed_ports' ${CFG} | cut -f2 -d':' | tr -d ' ')
if [[ -z ${closed_ports} ]]; then
    closed_ports=27017,27018,27019
fi

case $1 in

  start)
    ${ipset} list ${table_v4} &> /dev/null || ${ipset} create ${table_v4} hash:ip family inet
    ${ipset} list ${table_v6} &> /dev/null || ${ipset} create ${table_v6} hash:ip family inet6

    ${iptables} -A OUTPUT -p tcp -m set --match-set down4 dst -m multiport --ports ${closed_ports} -j REJECT --reject-with tcp-reset
    ${ip6tables} -A OUTPUT -p tcp -m set --match-set down6 dst -m multiport --ports ${closed_ports} -j REJECT --reject-with tcp-reset
    ;;

  stop)
    ${iptables} -D OUTPUT -p tcp -m set --match-set down4 dst -m multiport --ports ${closed_ports} -j REJECT --reject-with tcp-reset
    ${ip6tables} -D OUTPUT -p tcp -m set --match-set down6 dst -m multiport --ports ${closed_ports} -j REJECT --reject-with tcp-reset
    ;;
  *)
    ;;
esac
