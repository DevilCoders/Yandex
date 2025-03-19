#!/bin/bash

master_sign_pub=/etc/salt/pki/minion/master_sign.pub
ns_re='\[([0-9]+)\]'

if [[ $(readlink -f /proc/1/ns/pid) =~  $ns_re ]]; then
   init_ns=${BASH_REMATCH[1]};init_ns=${init_ns##*[};init_ns=${init_ns%]}
fi

for p in $(pgrep '^salt-minion$'); do
   p_dir=/proc/$p
   if test -e $p_dir && [[ "$(readlink -f $p_dir/ns/pid)" =~  :.$init_ns ]];then
      minion_pids[i++]=$p
   fi
done

if test -n "${minion_pids[*]}"; then
   while read p m; do
      p=${p//./,}; m=${m//-/0}; m=${m//./,}
      let cpu_usage+=$p
      let mem_usage+=$m
   done < <(ps --no-headers -o %cpu,%mem -m -p ${minion_pids[@]})
   if (($cpu_usage > 90)); then
      msg="2;minion eat $cpu_usage% cpu!"
   fi
   if (($mem_usage > 40)); then
      msg="2;minion eat $mem_usage% memory!"
   fi
   if ((${#minion_pids[@]} > 10)); then
      msg="2;here running ${#minion_pids[@]} minion!${msg#[0-9]}"
   fi
else
   msg="2;minion not alive!"
fi

if [[ "$(service salt-minion status 2>&1)" =~ stop ]];then
   msg="2;upstart say daemon not alive${msg#[0-9]}"
fi


# if failover enabled
if egrep -q '^[^#]*master_type[^#]+failover' /etc/salt/minion.d/minion.conf; then
   # and master_sign.pub not exists
   if ! test -e $master_sign_pub; then
      msg="2;failover enabled without master_sign.pub${msg#[0-9]}"
   elif ! openssl rsa -noout -text -pubin -in $master_sign_pub &>/dev/null; then
      msg="2;failed to load master_sign.pub${msg#[0-9]}"
   fi
fi

echo "${msg:-0;ok}"
