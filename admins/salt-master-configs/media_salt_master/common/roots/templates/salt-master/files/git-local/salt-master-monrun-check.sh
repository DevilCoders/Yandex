{%- set repos = [] -%}

{%- for repo in salt_master.git_local -%}
{%- do repos.append(repo.keys()[0]) -%}
{%- endfor -%}
MEMORY_THRESHOLD={{salt_master.get("monrun", {})["memory_threshold"]|default(50)}}
REPOS="{{ repos|join(' ') }}"

#!/bin/bash

if [[ "$1" =~ -d|--debug ]]; then
   set -x
else
   exec 2>/dev/null
fi

master_pids="`pgrep '^salt-master$'`"

if [[ "$master_pids" ]]; then
   for pid in $master_pids; do
      : $((master_rss_vm_kb+=`awk '/VmRSS/{print $2}' /proc/$pid/status||echo 0`));
   done
   total_memory=`awk '/MemTotal/{print $2}' /proc/meminfo`
   master_rss_vm_percent=$((master_rss_vm_kb/(total_memory/100)))
   if ((master_rss_vm_percent > MEMORY_THRESHOLD)); then
      lvl=2
      msg="${msg}${msg:+, }daemon ate 50% memory"
      echo "${lvl:-0};${msg:-ok}"
      exit 1;
   fi
else
   lvl=2
   msg="${msg}${msg:+, }daemon not alive"
   echo "${lvl:-0};${msg:-ok}"
   exit 1;
fi

if grep -q '^master_sign_pubkey:\s\+True' /etc/salt/master; then
   csync=$(command -v csync2)
   if test -n "$csync"; then
      if ! err=$($csync -xd 2>&1); then
         lvl=2
         msg="${msg}${msg:+, }csync2 ERROR: $err"
         echo "${lvl:-0};${msg:-ok}"
         exit 1;
      fi
   fi
fi

check_repo() {
  local lvl=0 msg="" mirror=/srv/media_salt_mirror_${1}

  if ! test -e $mirror/FETCH_HEAD; then
    lvl=2
    msg="${msg}${msg:+, }git ERROR: can't find media_salt mirror"
  else
    last_git_update_time=$(stat -c "%Y" $mirror/FETCH_HEAD)
    current_time=$(date +%s)
    # репа обновляется кроном из common/roots/templates/salt-master/files/git-local/salt-git-update.cron
    # в данный момент там */5 - каждые 5 минут
    time_Nminutes_ago=$((current_time - 600))
    if (($last_git_update_time < $time_Nminutes_ago));then
      lvl=2
      msg="${msg}${msg:+, }git ERROR: last update $((current_time - last_git_update_time)) seconds ago"
    else
      i=0
      while true; do
        if test -s $mirror/FETCH_HEAD; then
          break
        else
          sleep 1
          if ((i++, i>20));then
            lvl=2
            msg="${msg}${msg:+, }git ERROR: EMPTY FETCH_HEAD (git remote update failed)"
            break
          fi
        fi
      done
    fi
  fi
  echo "$lvl $msg"
}

IDX=0
STDOUTPUTS=()
for repo in $REPOS; do
  coproc { check_repo $repo; read; }
  STDOUTPUTS[IDX++]=${COPROC[0]}
done

for output in ${STDOUTPUTS[@]}; do
  read -u $output new_lvl add_msg
  if test -n "$add_msg"; then
      lvl=$((new_lvl>lvl?new_lvl:lvl))  # raise check level
      msg="${msg}${msg:+, }$add_msg"
  fi
done

echo "${lvl:-0};${msg:-OK}"
