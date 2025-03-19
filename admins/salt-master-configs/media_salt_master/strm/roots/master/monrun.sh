#!/bin/bash

if [[ "$1" =~ -d|--debug ]]; then
   set -x
else
   exec 2>/dev/null
fi

MEDIA_SALT_MIRROR=/srv/media_salt_mirror
master_pids="`pgrep '^salt-master$'`"

if [[ "$master_pids" ]]; then
   for pid in $master_pids; do
      : $((master_rss_vm_kb+=`awk '/VmRSS/{print $2}' /proc/$pid/status||echo 0`));
   done
   total_memory=`awk '/MemTotal/{print $2}' /proc/meminfo`
   master_rss_vm_percent=$((master_rss_vm_kb/(total_memory/100)))
   if ((master_rss_vm_percent > 50)); then
      lvl=$((lvl>1?lvl:2)) # set 2 if lvl < 2
      msg="${msg}${msg:+, }daemon ate 50% memory"
   fi

   if grep -q '^master_sign_pubkey:\s\+True' /etc/salt/master; then
      csync=$(command -v csync2)
      if test -n "$csync"; then
         if ! err=$($csync -xd 2>&1); then
            lvl=$((lvl>1?lvl:2)) # set 2 if lvl < 2
            msg="${msg}${msg:+, }csync2 ERROR: $err"
         fi
      fi
   fi
   if ! test -e $MEDIA_SALT_MIRROR/FETCH_HEAD; then
      lvl=$((lvl>1?lvl:2)) # set 2 if lvl < 2
      msg="${msg}${msg:+, }git ERROR: can't find media_salt mirror"
   else
      last_git_update_time=$(stat -c "%Y" $MEDIA_SALT_MIRROR/FETCH_HEAD)
      current_time=$(date +%s)
      time_2minutes_ago=$((current_time - 120))
      if (($last_git_update_time < $time_2minutes_ago));then
         lvl=$((lvl>1?lvl:2)) # set 2 if lvl < 2
         msg="${msg}${msg:+, }git ERROR: last update $((current_time - last_git_update_time)) seconds ago"
      else
         i=0
         while true; do
            if test -s $MEDIA_SALT_MIRROR/FETCH_HEAD; then
               break
            else
               sleep 1
               if ((i++, i>20));then
                  lvl=$((lvl>1?lvl:2)) # set 2 if lvl < 2
                  msg="${msg}${msg:+, }git ERROR: EMPTY FETCH_HEAD (git remote update failed)"
                  break
               fi
            fi
         done
      fi
   fi
else
   lvl=$((lvl>1?lvl:2)) # set 2 if lvl < 2
   msg="${msg}${msg:+, }daemon not alive"
fi

echo "${lvl:-0};${msg:-ok}"
