#!/bin/bash

if [[ "$1" =~ -d|--debug ]]; then
   set -x
else
   exec 2>/dev/null
fi

TICKET_NAME="`ls -1 /etc/nginx/ssl/tls/*key 2>/dev/null|head -n1`"
AGO="28 hours ago"
declare -A sha_sums

if test -s /etc/monitoring/tls-tickets.conf; then
   source /etc/monitoring/tls-tickets.conf
fi

if test -n "$TICKET_NAME" && test -e "$TICKET_NAME"; then
   TICKET_NUM=`ls -1 $TICKET_NAME* 2>/dev/null|wc -l`
fi

if (( ${TICKET_NUM:-0} < 3 )); then
   lvl=2
   msg="number tickets smaller then expected, $TICKET_NUM < 3"
else
   current_time=$(date +%s)
   some_time_ago=$(date +%s -d "$AGO")
   time_delta=$((current_time - some_time_ago))

   for num in {1..3}; do
      t_name=${TICKET_NAME}
      case $num in
         2) t_name=${t_name}.prev;;
         3) t_name=${t_name}.prevprev;;
      esac
      t_short_n=${t_name##*/}
      if test -e "$t_name"; then
         read ts tn <<<$(sha256sum $t_name)
         sha_sums[$ts]="${sha_sums[$ts]}${sha_sums[$ts]:+,}$t_short_n"

         last_update_time=$(stat -c "%Y" $t_name)
         expected_update_time=$(date +%s -d "$((time_delta * $num)) seconds ago" )
         if (($last_update_time < $expected_update_time));then
            lvl=$((lvl>1?lvl:2)) # set 2 if lvl < 2
            msg="${msg}${msg:+, }$t_short_n last update more then $((time_delta * num / 3600)) hours"
         fi
         if (( $(stat -c %s $t_name) != 48 )); then
            lvl=$((lvl>1?lvl:2)) # set 2 if lvl < 2
            msg="${msg}${msg:+, }$t_short_n size not equal 48 bytes"
         fi
      else
         lvl=$((lvl>1?lvl:2)) # set 2 if lvl < 2
         msg="${msg}${msg:+, }$t_name not found!"
      fi
   done
   if ((${#sha_sums[@]} < 3)); then
      lvl=$((lvl>1?lvl:2)) # set 2 if lvl < 2
      for s in ${!sha_sums[@]}; do
         if [[ ${sha_sums[$s]} =~ , ]];then
            msg="${msg}${msg:+, }${sha_sums[$s]} has same sha256sum!"
         fi
      done
   fi
fi

echo "${lvl:-0};${msg:-ok}"
