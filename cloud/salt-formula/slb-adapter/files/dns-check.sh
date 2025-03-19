#!/bin/bash

dig AAAA yandex.ru +time=10 +tries=2 +short >/run/nginx-monitor/dns-status.tmp

retcode=$?

if [[ "$retcode" -eq 0 ]]; then
    mv /run/nginx-monitor/dns-status{.tmp,}
else
    mv /run/nginx-monitor/dns-status.{tmp,err}
    rm /run/nginx-monitor/dns-status
fi
