#!/usr/bin/env bash

ssh cbb-production-yp-2.sas.yp-c.yandex.net \
    'awk '"'"'{print $6}'"'"' /logs/current-logs-nginx-cbb-access.log | sort -u' \
    | grep cgi-bin \
    >request_urls.txt

