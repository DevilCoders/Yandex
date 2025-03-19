#!/bin/bash
if [ -f /tmp/seclog-rsync-status ] ;  then
    ca=$(stat -c %Y  /tmp/seclog-rsync-status)
    cb=$(date '+%s')
    cc=$(($ca-$cb))
    if [ $cc -ge 86400 ]; then 
       echo "2; Rsync not runnig today. Please read https://st.yandex-team.ru/KP-174"; exit;
    fi 
    code=$(cat /tmp/seclog-rsync-status)
    if [ $code -ne "0" ]; then 
       echo "2; Rsync exit with code $code. Please read https://st.yandex-team.ru/KP-174"
    else 
       echo "0; OK"
    fi 
else
  echo "2; Rsync not runnig today. Please read https://st.yandex-team.ru/KP-174 "
fi
