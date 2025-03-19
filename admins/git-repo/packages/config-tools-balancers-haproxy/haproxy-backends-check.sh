#!/bin/bash

run_procedure=$1

get_stat_from_sock () {
    socat unix-connect:/tmp/haproxy-tools stdio
}

get_list_of_backends () {
    echo "show stat" | get_stat_from_sock | cut -d , -f 1 | egrep -v '(# pxname|stats|http)' | sort | uniq
}

get_percentage_of_down_rs_crit () {
    # clear var:
    get_percentage_of_down_rs_result=""

    # get list of backends, where 50% or more backend is down: 
    # (yeah, it is a holy shit, but...)
    get_percentage_of_down_rs_result=$(for i in $(get_list_of_backends); do 
	down=$(echo "show stat" | get_stat_from_sock | grep $i | grep -v BACKEND | cut -d , -f 18 | grep -c DOWN)
	up=$(echo "show stat" | get_stat_from_sock | grep $i | grep -v BACKEND | cut -d , -f 18 | grep -c UP)
	if [[ $up == "0" ]]; then 
            echo -n "$i "
        else 
	    percentage=$((${down}/${up}))
	    if [[ $percentage != "0" ]]; then 
		echo -n "$i "
	    fi
       fi
    done)

    # print result with monrun format: 
    if [[ $get_percentage_of_down_rs_result != "" ]]; then
	echo "2; $get_percentage_of_down_rs_result - 50% or more RS is DOWN"
    elif [[ $get_percentage_of_down_rs_result == "" ]]; then 
	echo "0; ok"
    fi
}

get_percentage_of_down_rs_achtung () {
    # clear var:
    get_percentage_of_down_rs_result=""

    # get list of backends, where 100% of backends is down: 
    get_percentage_of_down_rs_result=$(for i in $(get_list_of_backends); do 
	echo "show stat" | get_stat_from_sock | grep $i | grep BACKEND | cut -d , -f 18 | grep DOWN 1>/dev/null && echo -n "$i "
    done)

    # print result with monrun format: 
    if [[ $get_percentage_of_down_rs_result != "" ]]; then
	echo "2; $get_percentage_of_down_rs_result - ACHTUNG!! service is down here"
    elif [[ $get_percentage_of_down_rs_result == "" ]]; then 
	echo "0; ok"
    fi
}


if [[ $run_procedure == "get_percentage_of_down_rs_achtung" ]]; then 
    get_percentage_of_down_rs_achtung; 
elif [[ $run_procedure == "get_percentage_of_down_rs_crit" ]]; then 
    get_percentage_of_down_rs_crit;
else echo "2; please \$1 get_percentage_of_down_rs_crit or get_percentage_of_down_rs_achtung"
fi

