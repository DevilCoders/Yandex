#!/bin/bash

export ME=realtime-gc-monrun
export FIFO="${FIFO:-/var/tmp/$ME.fifo}"
export WATCH_LAST=3600
export HOSTNAME=`hostname -f`
export APP_NAME=${HOSTNAME%%[0-9]*}
export TS=$(date +%s)
export MONITORING_LOG_FILE=/var/log/yandex/${ME:-realtime-gc-monrun}.log
export MONITORING_MAX_LOG_SIZE=64
export JMAP_DIR="/var/tmp/fullgc"
export COUNTS_RESET_INTERVAL='1 day'
export YA_ENV=$(cat /etc/yandex/environment.type 2>/dev/null || echo dev)

mkfifo $FIFO 2>/dev/null

log(){
   local ts="$(date '+%F %T')" lineno=`caller 0`
   : ${lineno:="- $0 x"}
   printf -- "$ts:[$$] %s#%s: $@\n" ${lineno% *} >> $MONITORING_LOG_FILE
}; export -f log;
dlog() { if ${DEBUG:-false}; then log "$@"; fi; }; export -f dlog

die() {
  local command=$1 reason=$2
  local msg="$command failed with $reason"
  log "$msg"
  echo "2;$msg"
  exit 0
}; export -f die

conductor() {
    local method="$1" object="$2" format="${3:+?format=}$3"
    local uri="$method/$object$format"
    local cache_file="/var/tmp/conductor-cache/$uri"
    mkdir -p "${cache_file%/*}"

    local code answer

    local api="http://c.yandex-team.ru/api-cached"
    local curl_opts=" -fs --retry 2 --max-time 2 -w %{http_code}"
    code=$(curl $curl_opts "$api/$uri" -o "$cache_file~tmp" 2>/dev/null)

    if ((code == 200)); then
        mv -f "$cache_file~tmp" "$cache_file"  &>/dev/null
        answer="$(cat "$cache_file" 2>/dev/null)"
    elif ((code != 404)) && test -s "$cache_file"; then
        answer="$(cat "$cache_file" 2>/dev/null)"
    else  # тут кондуктор либо сломался, либо вернул 404
          # exit 1 в конце этой ветки не убъет основной скрипт, потому-что
          # эта функция будет запущена в sub shell-е
          # вызывающему нужно проверять exit код и реагировать на ошибку
          # при ошибке на stdin будет отправлен http код от кондуктора
          # и так же будет очищен кеш если http_code == 404
        ((code == 404)) && rm -f "$cache_file"
        echo $code
        exit 1
    fi
    echo "$answer"
}; export -f conductor;

# we need command conductor for initialization
PROJECT=${PROJECT:-$(conductor hosts2projects $HOSTNAME)} || die conductor "http=$PROJECT"
# Нельзя вызывать export на одной строке с die,
# потому-что export подавляет ошибку из conductor.
export PROJECT

rotate_logs(){
   local logfile=$1
   local max_size=$(( (1024**2)*${2:-64} )) # 64MB
   if test -s $logfile; then
      local logzise=$(stat -c "%s" $logfile)
      if (( $logzise > $max_size ));then
         log "Rotate log"
         pigz -f $logfile
      fi
   fi
}

tailer() {
  # for job management
  # set -x
  exec 9<>$FIFO     # Hold the fd open.
  if ! flock -n 9 ; then
     # Another instance is running
     exit 0
  fi

  set -o monitor

  local RE='Full GC'
  local RE_EXCLUDE='System.gc'
  local LOG_FILTER='*-gc*.log'
  local MAIN_PID=$$

  declare -A pids logs
  declare -g new_log_notifier
  declare -g update_notifier


  if test -s /etc/monitoring/$ME.conf; then
    source /etc/monitoring/$ME.conf;
  fi
  local WATCHED_DIR=${1:-${WATCHED_DIR:-/var/log}}
  if [ -z "$HPROF_DIR" ];then
      HPROF_DIR=${HPROF_DIR:-/var/spool/$PROJECT/$APP_NAME/tmp/}
  fi
  ## set defaults
  : ${OPEN_LOGS_NEWER_THEN:=3600}
  log "ATENTION!!! files oldest then $OPEN_LOGS_NEWER_THEN seconds will be ignored!"


  if ${DEBUG_STDERR:-false} ;then
    if ${DEBUG_SET_X:-false} ;then set -x; fi
    truncate -s0 $MONITORING_LOG_FILE.debug
    exec 3<>$MONITORING_LOG_FILE.debug  # now open log
    exec 1>&3                           # stdout to log
    exec 2>&1                           # stderr to stdout to log =)
  fi

  notifiers(){
    case $1 in
      update)
        { inotifywait -qqe modify /usr/sbin/realtime-gc-monrun.sh
          log "Main script updated, exiting"
          kill $2; } &
        update_notifier=$!
        ;;
      new_log)
        # sleep for wait will be runned before notifier die
        # not watch recursively in supplied directory
        ( sleep 0.01;inotifywait -qqe create $WATCHED_DIR;exit 81 ) &
        new_log_notifier="$!"
        dlog "Inotify wait for new logs on $WATCHED_DIR"
        ;;
    esac
  }

  mytailf(){
    local file="$1" inc="$RE" exc="$RE_EXCLUDE" special_case=""
    if ${HPROF_BEFORE_GC:-false}; then
        special_case="/Heap Dump [(]before full gc[)]/s#^#0000 #p;"
    fi

    # name example /var/log/music/worker-gc.pid913139.2016-06-09_17-35-23.log
    # should grab 913139 first full number from name.
    local pid="$(grep -o '[0-9]\+' <<<"${file##*/}"|head -n1)"
    tailf -0 $file|sed -run "/$inc/{$special_case/$exc/!s#^#${pid:-$file} #p}" >&9
  }

  cleanup(){
      local pid="$1"
      local file=pids["$pid"]
      unset pids["$pid"]
      unset logs["${file:-null}"]
  }

  killone(){
      # kill one tailer process, and all its children
      local sig="$1" pid="$2"
      dlog "$(pkill -"$sig" -P $pid 2>&1 && \
          echo "[$pid]taiers child killed"|| echo "[$pid]tailer have no child")"
      dlog "$(kill -"$sig" $pid && echo "[$pid]tailer killed"||\
          echo "[$pid]tailer probably died of grief")"
  }

  killer(){
    log "Killer got '$1' signal"
    if test -n "${!pids[*]}"; then
      for t in "${!pids[@]}" $new_log_notifier $update_notifier; do
        killone "$1" "$t"
      done
    fi
  }

  semargl() {
    local exit_code=$1 file
    local now=$(date +%s)
    local pid

    dlog "Harvest time, some child gone with exit code '$exit_code'"
    if ((exit_code == 138));then
        dlog "Probably debug via kill -USR1 $$"
    fi
    for pid in ${!pids[@]}; do
        file=${pids["$pid"]}
        if kill -0 $pid 2>/dev/null; then  # tailer alive
            #owner=$(ps --no-headers -p $pid -o ppid)
            local cmd="$(ps --no-headers p $pid)"
            if fgrep -q " tailer " <<<"$cmd"; then
                if test -e "$file"; then
                    dlog "[$pid] is still alive for $file"
                    if ((now - $(stat -c "%Y" "$file") > OPEN_LOGS_NEWER_THEN));then
                        log "[$pid] but file '$file' too old, kill tailer($pid)"
                        killone TERM "$pid"
                    else # tailer alive and file with actual mtime
                        continue
                    fi
                else
                    log "[$pid] alive, but file gonne, kill it tailer"
                    killone TERM "$pid"
                fi
            else
                log "[$pid] '$cmd' is not tailer, cleanup"
            fi
            # cleanup on this point work for all cases above within this if statement
            cleanup "$pid"
        else  # tailer not alive
            cleanup "$pid"

            if test -z "$file";then
                log "ERROR $pid die and have no file"
                continue
            fi
            if [[ "$file" =~ ^$WATCHED_DIR ]] && ! test -e "$file"; then
                # file in watched dir and alredy removed
                log "File '$file' removed"
                continue
            fi

            local code="$exit_code"
            if ((code == 300)); then
                # old bash exit code need retrive from exited child
                wait $pid; code=$?
            fi
            case $code in
                3|4|127|140)
                    log "tailer [$pid] gone, status [$code], clean: $file"
                    ;;
                *)
                    log "tailer [$pid] gone, status [$code], try re run: $file"
                    run_one_tailf $file
                    ;;
            esac
        fi
    done

    # new log watcher reup and tailer actualization section
    if ! kill -0 $new_log_notifier 2>/dev/null ||\
        ! ps --no-headers p $new_log_notifier|fgrep -q " tailer "; then

        dlog "Inotify event: new log"
        if [ "x$CLEAN_OBSOLETE_LOGS" == "xYES_CLEAN_IT" ]; then
            log_rotater $WATCHED_DIR
        fi
        runner $WATCHED_DIR

        dlog "[$new_log_notifier] new_log_notifier gonne, rerun"
        notifiers new_log
    fi

    dlog "Harvest time, end"
  } # end semargl

  run_one_tailf(){
      local file=$1 t_pid
      mytailf $file &
      t_pid=$!
      log "new tailer [$t_pid] for - $file"
      logs["$file"]=$t_pid
      pids["$t_pid"]="$file"
  }

  runner() {
    local wdir=$1 tailer_pid

    dlog "Runner find logs and run tailers"
    while read -d $'\0' file; do
      tailer_pid=${pids["${logs["$file"]:-null}"]}
      if test -z "$tailer_pid"; then
        dlog "Runner run tail with log '$file'"
        run_one_tailf "$file"
      fi
      done < <(find -L "$wdir" -type f -name "$LOG_FILTER" \
               -mmin -$((OPEN_LOGS_NEWER_THEN/60)) -print0)
  }
  log_rotater() {
    local wdir=$1

    dlog "Log rotater find and remove obsolete logs"
    while read -d $'\0' fname; do
      log "$(rm -vf "$fname")"
    done < <(find -L "$wdir" -type f -name "$LOG_FILTER" \
               -mmin +$((OPEN_LOGS_NEWER_THEN/60)) -print0)
  }

  trap '
      excode=$?;  trap - EXIT CHLD
      exec 9>&-
      killer KILL
      exit $excode
  ' INT TERM EXIT

  flinc(){
    local pid
    log "DEBUG main tailer pid is [$MAIN_PID]"
    log "DEBUG update_notifier is [$update_notifier]"
    log "DEBUG new_log_notifier is [$new_log_notifier]"
    for pid in ${!pids[@]}; do
        log "DEBUG [$pid] watch for ${pids["$pid"]}"
    done
  }

  trap flinc USR1


  log "Start tailers to tail log"
  runner "$WATCHED_DIR"
  log "Tailers started."

  dlog "Runner start tailers:"
  dlog "$(printf "%-6s\t%s\n" "PID" "FILE")"
  for p in "${!pids[@]}"; do
    dlog "$(printf "%-6s\t%s\n" "$p" "${pids[$p]}")"
  done

  # notifiers
  log "Run notifiers"
  notifiers update $MAIN_PID
  notifiers new_log

  ### XXX
  # main event loop
  #
  log "Waiting for tailers"
  version="$(tr -d '.' <<<"${BASH_VERSION:0:3}")"
  if ((version > 42 )); then
    log "New bash $BASH_VERSION($version), wait via wait -n"
    while true; do
      wait -n ${!pids[@]}
      semargl $?
    done
  else
    log "Old bash $BASH_VERSION($version), wait via trap CHLD signal"
    trap 'semargl 300' CHLD
    wait
  fi
  log "Tailers gone, exiting."
}; export -f tailer


jmapper() {
  local have_hprof have_auto_hprof fdate=$(date +%F_%H.%M.%S)
  # found fullgc, need jmap
  if test -s /etc/monitoring/$ME.conf; then
    source /etc/monitoring/$ME.conf;
  fi

  RSYNC_HOSTS=${RSYNC_HOSTS:-$(conductor groups2hosts ${PROJECT}-backup)} \
    || die conductor "http=$RSYNC_HOSTS"
  GROUP=${GROUP:-$(conductor hosts $HOSTNAME yaml|grep -Po "group:\s*\K.*")} \
    || die conductor "resolve group for $HOSTNAME"

  dlog "Jmapper started"

  local pid=$1
  if [[ "$pid" =~ ^/var/log ]];then
     dlog "Supplied pid is really full log file path, try gues pid from it"
    # try get actual pid from lsof
    java_pids="$(lsof $pid |awk '/^java/{print $2}')"
    if test -z "$java_pids"; then
      log "Jmapper can't find java process for log $pid, exiting"
      exit 0
    fi
    for p in $java_pids; do
      # assume gc in forked tack that mtime is newest
      pid_mtime=$(stat -c %Y /proc/$pid)
      if ((last_pid_mtime == 0));then
        pid=$p
      elif ((pid_mtime > last_pid_mtime));then
        pid=$p
      fi
      last_pid_mtime=$pid_mtime
    done
  fi

  if ! test -d "$JMAP_DIR";then
     log "$(mkdir -vp "$JMAP_DIR")"
  fi

  if ! ${HPROF_BEFORE_GC:-false}; then
      if ! [[ "$pid" =~ ^[0-9]+$ ]]; then
        log "Mission imposible, i can not find pid here => '$pid', exiting"
        exit 1
      fi
      local user=$(ps --no-headers -o euser $pid)
      dlog "$(chown -v "$user" "$JMAP_DIR" 2>&1)"
  else
      if ! pid=$(ls -t1 $JMAP_DIR/*hprof*|grep -m1 -Po 'java_pid\K\d+'); then
          pid="0000"  # we cannot get pid from hprof name, use 0000
      fi
  fi

  # only one jmap dump on host
  exec 7<>$JMAP_DIR/.flock
  if flock -n 7; then
    if ${HPROF_BEFORE_GC:-false}; then
      log "Ensure zk option's path $OPT_PATH"
      exists=$(zk exists $OPT_PATH 2>/dev/null)
      if [ "x$exists" == "xn" ];then
        for element in $(tr '/' ' ' <<<$OPT_PATH); do
          test_path="$test_path/$element"
          dlog "Ensure zk subpath $test_path"
          if [ "x$(zk exists $test_path 2>/dev/null)" == "xn" ];then
            if ! echo -n|zk create $test_path; then
              log "Failed to ensure HeapDumpBeforeFullGC zk option's path $test_path"
            fi
          fi
        done
      fi

      for f in $(echo $HPROF_DIR/*); do
        if [[ "$f" =~ [.]hprof([.][0-9]+)?$ ]] && test -f "$f"; then
          local current_zk_hbfgs="$(zk get $OPT_PATH 2>/dev/null)"
          log "Current state of HeapDumpBeforeFullGC at ${OPT_PATH}: $current_zk_hbfgs"
          if [[ "x$current_zk_hbfgs" != "xfalse" ]]; then
            log "Disable HeapDumpBeforeFullGC at $OPT_PATH"
            log "$(echo -n false|zk set $OPT_PATH 2>&1)"
          fi

          local fdate=$(date +%F_%H.%M.%S)
          wait_lsof=0
          while true; do
            let wait_lsof++
            if ! lsof "$f" &>/dev/null; then
              sleep 1.3s  # gave time for jmap close file
              fname=$(sed -r 's/[.]hprof([.0-9]+)$/\1.hprof/' <<<"${f##*/}")
              log "move $(mv -vf "$f" $JMAP_DIR/$fdate-${HOSTNAME%%.*}.$fname)"
              break
            else
              # if java do hprof dump we need guard it from ubic's watchdog
              if ((wait_lsof > 600)) ; then break; fi # jmap more then 10 minutes
              log "$(ubic stop ubic.watchdog 2>&1)"
              log "wait while java do hprof"
              sleep 1 # wait java done hprof
            fi
          done
        else
          dlog "skip strange file $(du -sh --time "$f")"
        fi
      done
    else
        log "$(ubic stop ubic.watchdog 2>&1)"
        log "Do jmap for:\n[$(ps --no-headers uwwwp $pid|sed 's/[%&*]\+//g')]\n"
        local hprof_file="$JMAP_DIR/$fdate-${HOSTNAME%%.*}.$pid.hprof"

        until sudo -u $user jmap -dump:format=b,file=$hprof_file $pid; do
          if ! fgrep -aq java /proc/$pid/comm;then
            log "$(ubic start ubic.watchdog 2>&1)"
            dlog "java process [$pid] gone. can't dump memory, exiting"
            exit 1
          fi
          if ((i++, i>${MAX_JMAP_RETRY:=300}));then
            log "$(ubic start ubic.watchdog 2>&1)"
            dlog "Max jmap retry ($MAX_JMAP_RETRY) reached, exiting"
            exit 1
          fi
          sleep 0.2
        done
    fi
    log "$(ubic start ubic.watchdog 2>&1)"

    if have_hprof="$(ls -1 $JMAP_DIR/*.hprof)"; then
      if [ "$RECIPIENTS" ]; then
        dlog "Send message to $RECIPIENTS"
        sendmail $RECIPIENTS < <(
          if ${HPROF_BEFORE_GC:-false}; then
            hdbgh="  HeapDumpBeforeFullGC here"
          fi
          echo "Subject: [${YA_ENV^^*}]$hdbgh Full GC on `hostname -f`"
          echo "To: ${RECIPIENTS// /,}"
          echo
          echo "Total jmap count:"
          echo "  For pid: ${hdbgh:-$(awk -F: '/^'$pid':/{n=$NF}END{print n}' $STAT_HOLDER)}"
          echo "  For host: ${hdbgh:-$(awk -F: '/^host:/{n=$NF}END{print n}' $STAT_HOLDER)}"
          echo
          echo "Total gc events: $(wc -l < $me_msg 2>/dev/null)"
          echo
          echo "Examine files: $(for f in $(echo $have_hprof); do echo -n "${f##*/}.gz "; done)"
          echo
          echo "Go to eclips:"
          echo "  eclipse01i.music.dev.yandex.net"
          echo "  eclipse01d.music.dev.yandex.net"
          echo
          echo "Or run on dev machine: getdump ${GROUP}/$(basename $(head -n1 <<<"$have_hprof")).gz ./hprofs/"
          echo "For help: getdump -h"
        )
      fi
    else
      if ${HPROF_BEFORE_GC:-false}; then
        log "Failed to create core! Exit."
        exit 1
      else
        log "No hprofs here, nothing to do, exit."
        exit 0
      fi
    fi
    for f in $(echo $have_hprof); do
        if ! test -f "$f"; then continue; fi
        log "Gzip file $f"
        pigz -f "$f"
        chmod 644 "$f.gz"
    done

    log "Upload cores and cleanup"

    if [ "$RSYNC_SECRETS" ];then
      source /etc/monitoring/$ME.secrets;
      export RSYNC_PASSWORD=$RSYNC_PASSWORD
    fi

    for rh in $RSYNC_HOSTS; do
      local rsync_request="$rh/${RSYNC_PATH:=cores}/${GROUP}/"
      local r_retry=0
      local rsync_ok=true

      log "$rh: Upload cores $(echo $JMAP_DIR/*gz)"

      until rsync_log=$(/usr/bin/rsync -aHv --timeout=$RSYNC_TIMEOUT --stats -h \
              $JMAP_DIR/*gz rsync://${RSYNC_USER}${RSYNC_USER:+@}$rsync_request)
      do
        if ((r_retry+=1, r_retry > ${RSYNC_MAX_RETRIES:=4})); then
           log "Failed to upload cores  (#${RSYNC_MAX_RETRIES} times)"
           continue 2
        fi
        log "Rsync failed, retry($r_retry): $rsync_log"
      done
      log "Rsync for '$rh' done: $rsync_log"
    done
    log "Remove local cores: $(rm -vf $JMAP_DIR/*.hprof.*)"
  else
    dlog "Another instance for jmapper is running"
    exit 0
  fi
}; export -f jmapper

jmapper_with_zk(){
  local pidn=$1
  local lockname="$(hostname -f|grep -Po '^[a-z-]+')_$(cat /etc/yandex/environment.type 2>/dev/null)"

  if test -s /etc/monitoring/$ME.conf; then
    source /etc/monitoring/$ME.conf;
  fi
  zks=${ZK_SLEEP_AFTER_JOB_IN_MINUTES:-10}

  log "Try run jmapper under zk lock with lockname=$lockname"
  # wait for zk lock 5 seconds
  zk-flock -c $ZKCFG -x 81 -w 5 $lockname "setsid bash -c 'jmapper $pidn && sleep ${zks}m' jmapper"
  local _es=$?
  if (($_es == 81));then
    log "Zk lock busy, do nothing..."
  elif (($_es != 0)); then
    log "Some thing went wrong, zk-flock exit with status: $_es"
  else
    log "Success exit from zk-flock."
  fi
}; export -f jmapper_with_zk

monrun() {
  local wdir="/tmp/realtime-gc-monrun"
  mkdir -p $wdir
  local tfile=`mktemp $wdir/$ME.gc-XXXXXX`
  local me_gc="$wdir/$ME.gc"
  export me_msg="$wdir/$ME.msg"
  declare -A pids_for_jmap

  # now re source config for monrun
  if test -s /etc/monitoring/$ME.conf; then
    source /etc/monitoring/$ME.conf;
  fi
  export STAT_HOLDER=$JMAP_DIR/.holder
  if ! test -e $STAT_HOLDER;then log "$(mkdir -vp ${STAT_HOLDER%/*})";touch $STAT_HOLDER;fi

  export HPROF_DIR=${HPROF_DIR:-/var/spool/$PROJECT/$APP_NAME/tmp/}
  export ZKCFG=${ZKCFG:-/etc/distributed-flock-realtime-gc-jmapper.json}
  export OPT_PATH="/$PROJECT/$YA_ENV/vm-options/$APP_NAME/cluster/HeapDumpBeforeFullGC"
  # конструкция с двумя вложенными $() ниже обязательно, нужно для автозамены
  # \n на пробелы, после чего их меняет tr на ,
  export ZOOKEEPER_SERVERS="$(tr ' ' ',' <<<$(jq .host[] < "$ZKCFG"|tr -d '"'))"

  if ! COUNTS_RESET_TIME=$(date +%s -d "$COUNTS_RESET_INTERVAL ago" 2>/dev/null); then
    dlog "Invalid format 'COUNTS_RESET_INTERVAL=$COUNTS_RESET_INTERVAL' use default '1 day'"
    COUNTS_RESET_TIME=$(date +%s -d "1 day ago")
  fi; export COUNTS_RESET_TIME

  local WTIME=$((TS - WATCH_LAST))

  while read -t 0.2 ; do
    echo "$REPLY" >> $tfile
    # first work is log name or pid, inserted by sed, see ps axf
    pids_for_jmap["${REPLY%% *}"]=1
  done < $FIFO

  touch $me_gc
  diff $me_gc $tfile|sed -ne '/^> /s//'"$TS"' /p' >> $me_msg
  mv $tfile $me_gc
  awk -v wtime=$WTIME 'wtime<$1' $me_msg > $tfile
  mv $tfile $me_msg

  if test -s "$me_msg";then
    if ${HPROF_BEFORE_GC:-false};then
      trigger=1 # here we always run jmapper, to avoid the lack of space

      if ! test -e $JMAP_DIR/.zk_opt_mtime; then touch $JMAP_DIR/.zk_opt_mtime; fi
      zk_opt_mtime=$(stat -c %Y $JMAP_DIR/.zk_opt_mtime)
      if ((zk_opt_mtime < TS - (RANDOM%600 + 600))) # check zk every 10 min + some random fraction from 10 min
        then touch $JMAP_DIR/.zk_opt_mtime
        dlog "Query zk $ZOOKEEPER_SERVERS for mtime of $OPT_PATH"
        if ! zk_opt_mtime=$(date +%s -d "$(zk stat $OPT_PATH|&grep -Po 'Mtime:\s+\K[^+]+')"); then
          dlog "Failed to query zk about mtime of $OPT_PATH, do not enable HeapDumpBeforeFullGC"
          zk_opt_mtime=$((COUNTS_RESET_TIME+1)) # avoid 0
        fi
      fi
      if ((zk_opt_mtime < COUNTS_RESET_TIME)); then
        local current_zk_hbfgs="$(zk get $OPT_PATH 2>/dev/null)"
        log "Current state of HeapDumpBeforeFullGC at ${OPT_PATH}: $current_zk_hbfgs"
        if [[ "x$current_zk_hbfgs" != "xtrue" ]]; then
          log "Enable HeapDumpBeforeFullGC at $OPT_PATH"
          log "$(echo -n true|zk set $OPT_PATH 2>&1)"
        fi
      else
        :  dlog "zk_opt_mtime=$zk_opt_mtime < COUNTS_RESET_TIME=$COUNTS_RESET_TIME, do not enable HeapDumpBeforeFullGC"
      fi
      if ls -1 $HPROF_DIR/*hprof* &>/dev/null; then
        ( setsid bash -c "jmapper 0000" jmapper &>/dev/null & )&
      fi
    else
      counts=$(awk '{sub(/:.*$/,n,$2);x[$2]++}END{for(i in x)print i"="x[i]}' $me_msg)
      local last_jmap_ts jmap_count_for_pid host_jmap_limit_ts jmap_count_for_host

      for c in $counts; do
        local jtr=${c##*=} jpid=${c%%=*}

        IFS=: read _ host_jmap_limit_ts jmap_count_for_host < <(grep -a "^host:" $STAT_HOLDER|tail -n1)
        IFS=: read _ last_jmap_ts jmap_count_for_pid < <(grep -a "^${jpid}:" $STAT_HOLDER|tail -n1)
        if ((host_jmap_limit_ts < COUNTS_RESET_TIME)); then
          log "Reset host's jmap count by interval $COUNTS_RESET_INTERVAL"
          jmap_count_for_host=0
          sed -i "/^host:/d" $STAT_HOLDER;echo "host:$TS:$((jmap_count_for_host))" >> $STAT_HOLDER
        fi
        if ((last_jmap_ts < COUNTS_RESET_TIME)); then
          log "Last jmap time too old, reset jmap's counts."
          jmap_count_for_pid=0
          sed -i "/^${jpid//\//\\\/}:/d" $STAT_HOLDER;echo "$jpid:$TS:$((jmap_count_for_pid))" >> $STAT_HOLDER
        fi
        if ((jtr >= JMAP_THRESHOLD)); then
          trigger=1
          if ${DO_JMAP:-true} && ((pids_for_jmap["$jpid"])); then
            log "Update jmap counts store at: $STAT_HOLDER"
            # limit number of heap dump for one jpid
            sed -i "/^${jpid//\//\\\/}:/d" $STAT_HOLDER;echo "$jpid:$TS:$((++jmap_count_for_pid))" >> $STAT_HOLDER
            # limit number of heap dump for host jpid
            sed -i "/^host:/d" $STAT_HOLDER;echo "host:$TS:$((++jmap_count_for_host))" >> $STAT_HOLDER

            if ((jmap_count_for_pid - 1 > ${CORE_COUNT_LIMIT_FOR_PID:=3})); then
              dlog "Pid's jmap limit [$CORE_COUNT_LIMIT_FOR_PID] for '$jpid' reached in '$COUNTS_RESET_INTERVAL'"
              break
            fi
            if ((jmap_count_for_host - 1 > ${CORE_COUNT_LIMIT_FOR_HOST:=12})); then
              dlog "Host's jmap limit [$CORE_COUNT_LIMIT_FOR_HOST] reached, in '$COUNTS_RESET_INTERVAL'"
              break
            fi
            ( setsid bash -c "jmapper_with_zk $jpid" jmapper &>/dev/null & )&
          fi
        fi
      done
    fi

    err=1
    if ${WARN_IS_OK:-false}; then
      err=0
    fi
    local num_gc=$(wc -l < $me_msg)
    if ((trigger))&&((num_gc > CALL_ADMIN_AT)); then
      err=2
    fi
    msg="$num_gc Full GC in the past $((WATCH_LAST/60)) minutes"
  fi
  echo "${err:-0};${msg:-OK}"

  rotate_logs $MONITORING_LOG_FILE $MONITORING_MAX_LOG_SIZE
  exit 0
}

( setsid bash -c "tailer $@" tailer &>/dev/null & )&

### Now go monrun check
monrun

# vim: sw=2 et
