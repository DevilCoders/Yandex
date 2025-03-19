#!/bin/bash

for daemon in bacula-{fd,sd,dir}; do
   if test -e /etc/init.d/$daemon; then
      if ! ret_text=$(service $daemon status 2>&1); then
         # status return non zero, some thing went wrong
         error_text+="$ret_text "
         error_code=2
      fi
   fi
done

echo "${error_code:-0};${error_text:-Ok}"
