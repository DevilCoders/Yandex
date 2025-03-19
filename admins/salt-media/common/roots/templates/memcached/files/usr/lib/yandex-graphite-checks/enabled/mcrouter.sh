#!/bin/sh
{% set port = salt['pillar.get']('memcached:mcrouter:port', 5000) -%}

echo 'stats all' | nc -w 5 0 {{port}} | perl -nE'if(/^STAT\s+([\w_]+)\s+(\d+\.?\d*)\s*$/){$k=$1;$v=$2;say"$k $v"}'
