#!/bin/sh
#
# Check for software RAID.
# Combined for Linux and FreeBSD servers.
#
me=${0##*/}     # strip path
me=${me%.*}     # strip extension
BASE=$HOME/agents
CONF=$BASE/etc/${me}.conf

PATH=/bin:/sbin:/usr/bin:/usr/sbin

die() {
        echo "PASSIVE-CHECK:$me;$1;$2"
        exit 0
}

linux_raid() {

  STAT_OID=/proc/mdstat
  CFG_FILE="/etc/mdadm/mdadm.conf /etc/mdadm.conf"
  err_c=0
  
  #Parce config
  arrays=""
  for f in $CFG_FILE ; do
    if [ -f $f ]; then
      arrays=`cat $f | grep "^ARRAY" | awk '{ print $2 } '`
      break
    fi
  done
  
  if [ "$arrays" = "" ]; then
    arrays=`cat $STAT_OID | tail -n +2 | grep raid | cut -d " " -f 1`
  fi
  
  for i in $arrays
  do
          # prepare data for analysis
          set -- `sudo /sbin/mdadm --detail $i | grep 'faulty\|remove\|degraded'`
          i=${i##*\\}
  
          [ "$*" ] || continue
  
          shift 4
  
          ERR="$i:$*"
  
          set -- `grep recovery $STAT_OID`
  
          [ "$*" ] && {
                  ERR="$ERR $2 $3 $4"
                  set --
                  set -- `echo $ERR | grep '%'`
                  [ "$*" ] && {
                          [ "$err_c" -eq "2" ] || err_c=1
                          ERR_MES="$ERR_MES $ERR"
  #                       err_c=1 
                          continue
  #                       die $err_c "$ERR"
                  }
          }
  
          case "$ERR" in
          *faulty*)       err_c=2
                          ;;
          *removed*)      set -- `grep -A1 $i $STAT_OID | tail -1`
                          err_c=2
                          ;;
          *)              err_c=2
                          ;;
          esac
          ERR_MES="$ERR_MES $ERR $*"
  
  done
  
  [ "$ERR_MES" ] || ERR_MES=Ok
  die $err_c "$ERR_MES"

}

freebsd_raid() {

  err_os=0
  ReleaseLimit=5
  VersionLimit=4
  EXT=0
  EXT2=0
  
  # Check version of release
  a=`uname -r | sed 's/[A-Za-z\-]//g' | cat | awk -v rel=$ReleaseLimit -v ver=$VersionLimit 'BEGIN { FS = "." }
  {
    ret = 0;
    CurrentRelease = $1;
    CurrentVersion = $2;
    if (CurrentRelease < rel)
    {
      ret = 1;
    }
    if (CurrentRelease == rel)
    {
      if (CurrentVersion < ver)
      {
        ret = 1;
      }
    }
    print ret;
  }'`
  [ $a -eq 0 ] || die 0 "freebsd current version less $ReleaseLimit.$VersionLimit"
  
  set -- `/sbin/gmirror status 2>/dev/null | grep -v Components | awk 'BEGIN { cc = 0; }
  {
    if ($0 ~ /mirror/)
    {
      if (cc != 0)
      {
        printf "\n";
      }
    }
    for (i = 1; i <= NF; i++)
    {
      printf "%s ", $i;
    }
    cc = cc + 1;
  } END { printf "\n"; }' | awk 'BEGIN { ext = 0; }
  {
    if ($0 !~ /COMPLETE/)
    {
      if ($0 ~ /%/)
      {
        if (ext < 1 ) ext = 1;
        err_str = err_str $0;
      }
      else
      {
        if ($0 ~ /DEGRADED/)
        {
          ext = 2;
          err_str = err_str $0;
        }
      }
    }
  } END { printf "%d %s", ext, err_str; }'`
  
  EXT=$1
  shift
  err_str="$*"
  
  set -- `/sbin/gstripe status 2>/dev/null | grep -v Components | awk 'BEGIN { cc = 0; }
  {
    if ($0 ~ /stripe/)
    {
      if (cc != 0)
      {
        printf "\n";
      }
    }
    for (i = 1; i <= NF; i++)
    {
      printf "%s ", $i;
    }
    cc = cc + 1;
  } END { printf "\n"; }' | awk 'BEGIN { ext = 0; }
  {
    if ($0 !~ /UP/)
    {
      if ($0 ~ /DEGRADED/)
      {
        ext = 2;
        err_str = err_str $0;
      }
    }
  } END { printf "%d %s", ext, err_str; }'`
  
  EXT2=$1
  shift
  err_str2="$*"
  
  C=$EXT
  MESS="$err_str $err_str2"
  [ $EXT2 -gt $C ] && C=$EXT2
  [ $C -eq 0 ] && MESS="Ok"
  
  # ZFS support
  status=0
  msg=""
  zpool_list="zpool list -H -o name"
  zpool_status="zpool status"

  : ${zpool_counters_prio=1}
  
  if [ -n "${DEBUG:-}" ]; then
  	zpool_list="echo $DEBUG"
  	zpool_status="cat"
  fi
  
  pools=$(${zpool_list} 2> /dev/null)
  for pool in ${pools}; do
    _msg="$(${zpool_status} "${pool}" | awk \
      -v counters_prio="${zpool_counters_prio}" '
  BEGIN {
    state = "";
    rebuild = 0;
    status = 0;
    on = 0;
  } {
    if ($1 == "pool:")
      pool = $2;
    if ($1 == "state:" && $2 != "ONLINE")
      state = $2;
    if (($1 == "status:" && $0 ~ /currently being resilvered/) || ($0 ~ /scrub in progress/)) {
      rebuild = 1;
      status = 1;
    }
    if (rebuild) {
      if ($1 == "scrub:" && $0 ~ / in progress /)
        printf("%s (rebuilt %s) ", pool, $7);
      if ($0 ~ / done$/) {
        printf("%s (rebuilt %s) ", pool, $3);
      }
    } else {
      if ($1 == pool) {
        on = 1;
        spare = 0;
      }
      if (!$0)
        on = 0;
      if ($1 == "spares")
        spare = 1;
      if (on && $1 ~ /(ad|ada|da|mfid)[0-9]/) {
        if (!spare && $2 != "ONLINE") {
          printf("%s (%s) ", $1, $2);
          status = 2;
        }
        if (spare && $2 != "AVAIL" ) {
          printf("%s (%s) ", $1, $2);
          status = 2;
        }
        if ($3 || $4 || $5) {
          printf("%s (", $1);
          if ($3) {
            printf("read: %s", $3);
            if ($4 || $5)
              printf(" ");
          }
          if ($4) {
            printf("write: %s", $4);
            if ($5)
              printf(" ");
          }
          if ($5)
            printf("cksum: %s", $5);
          printf(") ");
          if (status != 2)
            status = counters_prio;
        }
      }
    }
  } END {
    if (state && !rebuild) {
      status = 2;
      printf("%s (%s) ", pool, state);
    }
    if (status == 2)
      printf("crit")
    else if (status == 1)
      printf("warn")
  }
  ')"
  
    msg_status="${_msg##* }"
    if [ "${msg_status}" = "warn" -a ${status} -eq 0 ]; then
      status=1
    elif [ "${msg_status}" = "crit" ]; then
      status=2
    fi
    _msg="${_msg%% warn}"
    _msg="${_msg%% crit}"
    if [ -n "${_msg}" ]; then
      msg="${msg}${msg:+ }${_msg}"
    fi
  done
  
  if [ $status -gt 0 ]; then
    if [ $C -eq 0 ]; then
      MESS="$msg"
    else
      MESS="$MESS $msg"
    fi
    if [ $status -gt $C ]; then
      C=$status
    fi
  fi
  
  # Print result
  die $C "$MESS"

}

#
# check openvz CT via yandex-lib-autodetect-environment package
#
if [ -f /usr/local/sbin/autodetect_environment ] ; then
        . /usr/local/sbin/autodetect_environment >/dev/null 2>&1 || true
        if [ $is_virtual_host -eq 1 ] ; then
                die 0 "OK, openvz CT, skip raid checking"
        fi
fi

[ -s $CONF ] && . $CONF

case `uname` in
  FreeBSD) freebsd_raid
  ;;
  Linux)   linux_raid
  ;;
  *) die 0 "Unknown OS type"
esac

