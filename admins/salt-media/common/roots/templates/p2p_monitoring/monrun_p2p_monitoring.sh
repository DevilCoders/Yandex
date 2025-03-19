#!/bin/bash

check_name=$1; shift
declare -A stats error obsolete

case $check_name in
  age*)
    if [[ $1 =~ [0-9]+ ]]; then
      _utime=$1; shift
    fi
    len=$#
    for ((c=0;c<$len;c++));do
      # utime? (utime=$utime^)?dir=$dir^files=$files
      unset utime dir files
      eval ${1//^/ }
      shift
      utime=${utime:-${_utime:-2880}}
      files=( ${files//,/ } )
      if [ -d $dir ]; then
        for i in "${!files[@]}"; do
          for f in `find $dir -mmin +$utime -iname ${files[$i]} -print`; do
              obsolete[$f]="`basename $f`(`stat --printf=%y $f|cut -c-16`)"
          done
          if [ "${obsolete[*]}" ];then
            ifs=$IFS;IFS=,
            stats[$dir]="${obsolete[*]}"
            IFS=$ifs
          fi
        done
        obsolete=()
      else notexists[$c]=$dir; fi
    done
    if [ "${stats[*]}" ];then
      for d in "${!stats[@]}"; do
        msg+="$d: ${stats[$d]} "
      done
      msg="2;Obsolete files: $msg"
    fi
    if [ "${notexists[*]}" ];then
      ifs=$IFS;IFS=,
      msg="${msg:-1;}Not existing directories: ${notexists[*]}"
      IFS=$ifs
    fi
    echo ${msg:-0;OK}

    ;;
  client)
    clients=( `/usr/bin/start_stop_p2p_client list 2>/dev/null` )

    eval stats=( `start_stop_p2p_client status 2>/dev/null|\
      awk -F'|' '/^\| [0-9]/{gsub(/ /,l,$4);gsub(/ /,l,$3);print "["$3"]="$4}'` )

    if [ "${clients[*]}" ]; then
      for client in "${clients[@]}"; do
        if [[ "${stats[$client]}" != "RUNNING-ENABLED" ]]; then
          error[$client]=" ${client}: ${stats[$client]:-Unknown}"; fi
      done
    else
      echo "1;Can't get clients list `/usr/bin/start_stop_p2p_client list 2>&1| tr '\n' ' '`"
      exit 0
    fi

    if [ "${error[*]}" ]
      then IFS=,; echo "2;Clients status:${error[*]}"
      else echo "0;OK"    ; fi

    ;;
  *)
    echo "1;Usage: `basename $0` (client|age_<checkname> <utime> <dir> <file...>)"
    ;;
esac
