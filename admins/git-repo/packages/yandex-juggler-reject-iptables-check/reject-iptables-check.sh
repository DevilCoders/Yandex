#!/bin/bash                                                                                                                                                                                            

status=not 

/sbin/iptables-save 2>/dev/null | grep 1>/dev/null REJECT && status=rejected
/sbin/ip6tables-save 2>/dev/null | grep 1>/dev/null REJECT && status=rejected

if [[ ${status} != "not" ]]; then 
	echo "2; found reject rules"
elif [[ ${status} == "not" ]]; then
	echo "0; ok"
fi

