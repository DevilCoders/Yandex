#! /bin/bash

if ! [ "x$3" = "x" ]; then
  echo "Too many arguments, exiting."
  exit 1
fi
case $1 in
  "")
    echo "You must specify argument. Use table.sh --help for usage."
    exit 1
  ;;
  "--help")
    echo -e "		This script generates table of currently avialiable mirrors
		in Wiki syntax and writes results to file. You must specify
		output file by -f or --file argument.
		Usage: ./table.sh <-f|--file> outfile"
    exit 0
  ;;
  "-f"|"--file")
    out_file="$2"
  ;;
  *)
    echo "Unknown argument. Exiting."
    exit 1
  ;;
esac

if [ -f $out_file ]; then
  rm $out_file
fi

clean_line="`cat /etc/cron.d/mirror-scripts-sync | grep -n "#.*clean" | sed  's/\:.*$//g'`"
if [ "$clean_line" -eq "$clean_line" ] 2>/dev/null; then
  list="$(head -n $(expr $clean_line - 1) /etc/cron.d/mirror-scripts-sync | grep "/usr")"
else
  list="$(cat /etc/cron.d/mirror-scripts-sync | grep "/usr")"
fi

echo "|| Name | URL | Port | Method | cron rule | Last sync date ||" >> $out_file
echo "|| opensuse/repositories | none | 873 | passive | none | `date` ||" >> $out_file
while read i; do

  j="`echo $i | sed 's@.*/usr@/usr@g' | awk '{print $1}'`"
  tmpfile=`cat $j`
  raw_port_str="`echo \"$tmpfile\" | grep \"SYNC_REPO_PORT\" | grep -v \"\\$SYNC_REPO_PORT\" | awk -F'\"' '{ print $2 }'`"
  if [ "x`echo \"$tmpfile\" | grep \"simple_sync\|debian_sync\"`" = "x" ]; then
    if [ "$raw_port_str" = "80" ] || [ "$raw_port_str" = "8080" ]; then
      proto="http"
      port="$raw_port_str"
      URL="`echo \"$tmpfile\" | grep HOST | awk -F'\"' '{ print $2 }'`"
    else
      port="Unknown proto"
      URL="`echo \"$tmpfile\" | grep HOST | awk -F'\"' '{ print $2 }'`::`echo \"$tmpfile\" | grep MODULE | awk -F'\"' '{ print $2 }'`"
    fi
  else
    proto="rsync"
    URL="`echo \"$tmpfile\" | grep HOST | awk -F'\"' '{ print $2 }'`::`echo \"$tmpfile\" | grep MODULE | awk -F'\"' '{ print $2 }'`"
    if [ "x`echo \"$tmpfile\" | grep \"SYNC_REPO_PORT\"`" = "x" ]; then
      port="873"
    else
      port="$raw_port_str"
    fi
  fi

  name="`echo \"$tmpfile\" | grep REL_LOCAL_PATH | awk -F'\"' '{ print $2 }'`"
  if [ -f /mirror/$name/.mirror.yandex.ru ]; then
    sync_date=`date -d "$(cat /mirror/$name/.mirror.yandex.ru)"`
  else
    sync_date="No sync date"
  fi

  cron_rule="`echo "$i" | sed 's@ > /dev/null 2>&1@@g'`"

  if [ "$name" = puppyrus ]; then
    row="|| $name | none | 873 | passive | none | $sync_date ||"
  else
    row="|| $name | $URL | $port | $proto | $cron_rule | $sync_date ||"
  fi

  echo "$row" >> $out_file

done << EOF
$list
EOF


file="`cat $out_file | sort`"
rm $out_file


echo "#|" >> $out_file
echo "$file" >> $out_file
echo "|#" >> $out_file
