#!/bin/bash

len(){ echo ${#1};}
for i in "$@"; do case $i in
   -d*)dlevel=`len ${i//[-abce-z]/}` verbose=v; ((dlevel>4))&&set -x;;
   *) echo "Usage: $0 [-d[d[d[d]]]] - -d for debug";exit 0;;
esac;done;

r="[31m";g="[32m";o="[33m";b="[34m";m="[35m";p="[36m";w="[0m"
dlog(){
   local err=1;warn=3;info=6;debug=7 white=7
   local er=1; in=2;  de=3; wa=2 gr=2 wh=1
   local l=$1 ts="[`date '+%F %T'`]"; shift;
   ((dlevel < ${l:0:2}))&&return 0
   lineno=`caller 0`
   printf -- "$ts: ${o}line #%-4s ${g}%-8s$w: [3${!l}m$@$w\n" ${lineno% *}
}

DAEMON_CONF="/etc/yandex/statbox-push-client/push-client.yaml"
declare -A _ERRORS
TS=`date +%s`
LAST_SEND_LIMIT=3600

# '/var/log/music/web-likes.log'
log_re="^ '(/var/log/.*/([^/]+).log)':$"
# last_send_time: 1428087932 (03.04.2015-22.05.32)
send_time_re='last_send_time:\s+([0-9]+)\s+(\([^)]+\))'
# status: ok
status_re='^status:\s+(.*)$'
inode_re='inode:\s+([0-9]+)'
status_error_re='((ERROR|/var/log.*stat\(\):).*$)'

. /etc/default/push-client &>/dev/null||:
. /etc/monitoring/push-client-status.conf &>/dev/null||:

dlog white "
Check errors in logs        : CHECK_LOGS=${CHECK_LOGS=false}
                            : CHECK_LOGS_LAST=${CHECK_LOGS_LAST:=60}
                            : CHECK_LOGS_TRESHOLD=${CHECK_LOGS_TRESHOLD:=10}
Check 'push-client --status': CHECK_STATUS=${CHECK_STATUS=true}
Check inode                 : CHECK_INODE=${CHECK_INODE=false}
Check last send time        : CHECK_TIME=${CHECK_TIME=false}
Check errors in status      : CHECK_ERRORS=${CHECK_ERRORS=true}
Check last line with status : CHECK_PUSH_STATUS=${CHECK_PUSH_STATUS=true}
Check tvm error in logs     : CHECK_TVM=${CHECK_TVM=true}
                            : CHECK_TVM_LAST=${CHECK_TVM_LAST:=60}
                            : CHECK_TVM_TRESHOLD=${CHECK_TVM_TRESHOLD:=2}
"

error_push(){ _ERRORS["$1"]+="${_ERRORS["$1"]:+,}${@:2:$#}"; }
bname(){ local n=${1%.*}; echo ${n##*/}; }
check_param(){ ((dlevel > 0))||[[ "${1,,}" =~ (true|o[kn]|yes|enable) ]];}

num_proc=$(pgrep -fc '^/usr/bin/push-client ' 2>/dev/null)
dlog info "Found #$num_proc push-clinet process"
if  ((${num_proc:-0} <= 0)); then
   echo "2;Daemon not running"
   exit 1;
fi

for cfg in $DAEMON_CONF; do
   dlog info "Process config $cfg"
   conf_name=`bname $cfg`
                        # -w process are watch dog, skip it
   if ! pgrep -f "push-client[^/w]+-c[^/w]+$cfg" &>/dev/null; then
      dlog err "Worker for "'`'$cfg"' down!"
      down+="${down:+,}$conf_name"
      continue
   fi

   status_text="$(push-client --status -c $cfg 2>&1)"
   exit_status=$?
   if ((exit_status > 0));then _st="${r}Error";else _st="${g}Ok"; : "${w}";fi
   dlog info "Status '$_st' for worker '$conf_name'${w}`((dlevel>3))&&{
      echo "\n$status_text";}`"

   if check_param "${CHECK_STATUS}";then
      while read -d $'\n' line ; do
         line=${line#*]: }
         if [[ $line =~ $log_re ]];then
            dlog debug "Found log name: $line"
            log_name=${BASH_REMATCH[2]}
            log_file=${BASH_REMATCH[1]#/var/log/}
            continue
         fi
         if check_param "${CHECK_INODE}";then
            if [[ $line =~ $inode_re ]];then
               dlog debug "Found inode in: $line"
               inode_push=${BASH_REMATCH[1]}
               inode_real="`stat -c %i /var/log/$log_file 2>/dev/null`"
               if ((inode_real != inode_push));then
                  dlog warn "Inode from status '$conf_name' mismatch real file: $log_file"
                  error_push 'inode mismatch' "$log_file"
                  : $((ERR=ERR<1?1:ERR))  # monrun warning
               fi
            fi
         fi
         if check_param "${CHECK_TIME}";then
                      # last_send_time: 1428087932 (03.04.2015-22.05.32)
            if [[ $line =~ $send_time_re ]];then
               dlog debug "Found send time: $line"
               if ((TS - ${BASH_REMATCH[1]} > LAST_SEND_LIMIT)); then
                  dlog err "Long time no sending: $log_file - ${BASH_REMATCH[2]}"
                  error_push 'Long time no sending' "$log_file${BASH_REMATCH[2]}"
                  : $((ERR=2))  # monrun error
               fi
            fi
         fi
         if check_param "${CHECK_ERRORS}";then
            if [[ $line =~ $status_error_re ]];then
               dlog err "$line"
               error_push "${BASH_REMATCH[1]}"
               : $((ERR=ERR<1?1:ERR))  # monrun warning
            fi
         fi

         if [[ $line =~ $status_re ]];then
            stext=${BASH_REMATCH[1]}
            if check_param "${CHECK_PUSH_STATUS}";then
               dlog info "$line"
               if ! [[ $stext =~ ^ok$ ]];then
                  : $((ERR=2))  # monrun warning
                  dlog err "Worker $conf_name: '$line'"
                  _ERRORS['status']="$stext!"
               fi
            fi
            if ((ERR > 0)); then
               if [ "${!_ERRORS[*]}" ];then
                  dlog warn "Check for '$conf_name' passed with errors!"
                  for l in "${!_ERRORS[@]}";do
                     conf_status+="$l: ${_ERRORS[$l]}!"
                  done
               fi
            fi
            _ERRORS=()
         fi
      done <<<"$status_text"
   fi
   if check_param "${CHECK_LOGS}";then
      _errl=`timetail -n${CHECK_LOGS_LAST} /var/log/statbox/${conf_name}.log|\
                                    grep -c "ERR:\|EMERGE:"`
      if (( $_errl > 0 ));then
         dlog warn "Found $_errl error in log statbox/$conf_name.log"
      fi
      if (( $_errl > CHECK_LOGS_TRESHOLD ));then
         : $((ERR=2))  # monrun error
         dlog err "$_errl errors in statbox/$conf_name.log (treshold $CHECK_LOGS_TRESHOLD)!"
         conf_status+="Errors in log: $_errl!"
      fi
   fi
   if grep -q "proto: pq" $cfg; then
       if check_param "${CHECK_TVM}";then
          _errl=`timetail -n${CHECK_TVM_LAST} /var/log/statbox/${conf_name}.log|\
              grep -cE "(BROKEN_SIGN)|(bad tvm_secret)|(Malformed TVM)"`
          if (( $_errl > 0 ));then
             dlog warn "Found $_errl TVM errors in log statbox/$conf_name.log"
          fi
          if (( $_errl > CHECK_TVM_TRESHOLD ));then
             : $((ERR=2))  # monrun error
             dlog err "$_errl TVM errors in statbox/$conf_name.log (treshold $CHECK_TVM_TRESHOLD)!"
             conf_status+="TVM Errors in log: $_errl!"
          fi
       fi
   fi
   result_status+=${conf_status:+${result_status:+| }"[$conf_name]:$conf_status"}
   conf_status=""
done

if [ "$down" ];then
   ERR=2
   result_status="Push client down for: $down! $result_status"
fi

echo "${ERR:-0};${result_status:-OK}"
