#!/bin/bash
cat iptables | ip6tables-restore
cat iptables | grep -v "2a02:6b8:0:1a16::2:7" | iptables-restore

