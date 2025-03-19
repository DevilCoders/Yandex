#!/bin/bash
# vim: ft=awk

TIME=300
LIMIT=1
FILTER=""  # by default do not FILTER


me=${0##*/}; me=${me%.*}
CONF=/etc/monitoring/$me.conf    
if [ -s $CONF ];then . $CONF ; fi

while getopts ":f:n:l:h" opt; do case $opt in
    n)
      TIME="$OPTARG"
      ;;
    f)
      FILTER="$OPTARG"
      ;;
    l)
      LIMIT="$OPTARG"
      ;;
    h)
      echo " Usage: $0 [ -f FILTER ] [ -n TIME ] [ -l LIMIT ]"
      echo "    -h  show this help"
      echo "    -f  filter errorlog entries by PCRE regex (default: '')"
      echo "    -n  time for timetail (default: 300)"
      echo "    -l  errors threshold (default: 1)"
      exit 0
      ;;
    '?')
      echo "Invalid option: -$OPTARG" >&2
      exit 1
      ;;
    :)
      echo "Option -$OPTARG requires an argument." >&2
      exit 1
      ;;
esac; done

if ! test -e /var/log/nginx/error.log; then
    echo "0;OK error.log not found"
    exit 0
fi

timetail -n$TIME -r '(20\d\d/\d\d/\d\d \d\d:\d\d:\d\d)' /var/log/nginx/error.log 2>/dev/null|\
    if test -z "$FILTER";then cat;else grep -P -v "$FILTER";fi|\
    mawk -v limit="$LIMIT" '
/(emerg|alert|crit|error)/{
  if (NF > 5) {
    x[$6]++
  } else {
    x[$NF]++
  }
}
END{
  for (e in x) {
    if (substr(e, 1,3) == "\"/v") {
      listing++
      continue
    }
    if (x[e] > limit) {
      if (msg) {
        msg=msg", "
      }
      msg=msg""x[e]" "e
    }
  }
  if (listing > limit){
    if (msg) {
      msg=msg", "
    }
    msg=msg""listing" listing"
  }

  if (msg) {
    msg="2;"msg
  } else {
    msg="0;Ok"
  }
  print msg
}
' 
