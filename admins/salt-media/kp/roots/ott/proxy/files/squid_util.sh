#!/bin/sh

squidclient -h localhost cache_object://localhost/ mgr:utilization | \
  perl -ne 'if(/^Totals since cache startup:$/../^$/){if(/(client_http.requests|client_http.hits|client_http.errors|client_http.kbytes_in|client_http.kbytes_out|cpu_time) = ([\d|\.]+)/g){$2=~tr/a-z//;print"$1 $2\n";}}'
