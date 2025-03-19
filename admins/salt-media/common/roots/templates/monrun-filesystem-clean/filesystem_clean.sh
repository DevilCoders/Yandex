#!/bin/bash
#
# Provides: mulca_filesystem_clean



me=${0##*/}    # strip path
me=${me%.*}    # strip extension


die() {
    echo "$1;$2"
    exit 0
}

code=0;
msg="OK"
declare -a unclean_fs
# cannot use a pipe here, because while read will run in a subshell
while read drive fs ; do
if [ "$fs" != "ext4" ]; then
    if [ $code -lt 1 ]; then code=1; fi
    if [ "$msg" = "OK" ]; then
        msg="$drive is $fs"
    else
        msg="$msg; $drive is $fs"
    fi
else
    state=$(sudo /sbin/tune2fs -l $drive 2>/dev/null | grep "Filesystem state" | awk '{ print $NF }')
    if [ -z $state ]; then
        state='clean'
    fi
    if [ $state != "clean" ]; then
        unclean_fs+=( $drive )
    fi
fi
done < <(grep -E '(/u|storage/)[0-9]+' /etc/mtab | cut -f 1,3 -d ' ')

if [ ${#unclean_fs[@]} -gt 0 ]; then
    code=2
    if [ "$msg" = "OK" ]; then
        msg="bad fs on ${unclean_fs[@]}"
    else
        msg="$msg; bad fs on ${unclean_fs[@]}"
    fi
fi

drives_with_lost_found="$(
  sudo \
    find \
      /srv/storage/ \
      -type d \
      -name 'lost+found' \
      -not -empty \
      2>/dev/null | # list non-empty 'lost+found' directories in /srv/storage/*/
    awk -F '/' '{print $4}' | # /srv/storage/35/lost+found -> 35
    sort -nu | # sort and drop duplicates: 35\n3\n35 -> 3\n35
    paste -d , -s - # join all lines to ','-delimited string: 3\n35 -> 3,35
)" # expected result is either empty string, or comma-separated list of numbers: 1,22,34,112,...

if [ -n "${drives_with_lost_found}" ]; then
  code=2
  if [ "$msg" = "OK" ]; then
          msg="'lost+found' files detected on storages ${drives_with_lost_found}"
      else
          msg="$msg; 'lost+found' files detected on storages ${drives_with_lost_found}"
      fi
fi

die "$code" "$msg"
