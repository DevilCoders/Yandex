#!/bin/bash

export location="MAN"
export master=$location"_WEB_BASE"
export slaves="_WEB_PLATINUM_JUPITER_BASE)\|_WEB_TIER1_JUPITER_BASE)\|_WEB_CALLISTO_CAM_BASE)";
./utils/pregen/find_most_memory_free_machines.py -s $master -a show | grep Instance | awk '{print $2":"$4":"$6}' | awk -F ':' '$5>0{print $1" "$3" "$5}' | grep "$slaves" | awk '{print $1" "$3}' | sort -k1 | awk -f group_by_sum.awk | sort -k1 | awk '{if($2>440){print $1" "$2}else{print $1" 440"}}'> host_base_sum_power
./utils/common/dump_hostsdata.py -s $master -i name,power,ncpu | sort -k1 > host_power_cores
join host_base_sum_power host_power_cores | awk '{print $1" "int($2/$3*($4-4))}' > result.man
rm host_base_sum_power host_power_cores
