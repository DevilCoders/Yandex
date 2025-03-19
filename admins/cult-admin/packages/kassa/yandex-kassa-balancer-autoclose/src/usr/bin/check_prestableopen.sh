#!/bin/sh

(iptables-save  | grep -s  '\-A INPUT \-s 10.0.0.0\/8 \-p tcp \-m multiport \-\-dports 80\,443 \-j DROP' > /dev/null ) && ( ip6tables-save  | grep -s '\-A INPUT \-s fd00\:\:/8 \-p tcp \-m multiport \-\-dports 80\,443 \-j DROP' > /dev/null) &&  echo '0; OK' || echo '2; Rules not found'
