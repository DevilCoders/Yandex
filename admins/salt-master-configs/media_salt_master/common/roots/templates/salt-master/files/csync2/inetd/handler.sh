#!/bin/bash

#socket_inode=$(readlink -f /proc/self/fd/0|awk -F: '{print substr($NF,2,length($NF)-2)}')
#fgrep -h $socket_inode /proc/net/tcp{,6}

read -t 0.1 -d 0 cmd
r_ip=$(lsof -i -a -p $$ -d 0|sed -rn 's/[^>]+>\[?([^:]+).*/\1/p')

if [[ "$cmd" =~ "clean-key" ]]; then
  if [[ "$r_ip" =~ "yandex" ]]; then
    salt-key -yd $r_ip
  else
    echo "Forbidden"
  fi
elif [[ "$cmd" =~ "csync2" ]]; then
  echo "Your name: $r_ip"
  csync2 -m $@
  csync2 -xv
else
  echo "Forbidden"
fi
