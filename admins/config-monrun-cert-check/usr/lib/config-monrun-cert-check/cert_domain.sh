#!/bin/bash

err=

cert_path=$1

# include default cert pathes
. /usr/lib/config-monrun-cert-check/cert_path.sh

#define variable cert_domain
cert_domain=0
if [ -f /etc/default/config-monrun-cert-check ]; then
	source /etc/default/config-monrun-cert-check;
fi

if [ x"$cert_path" = "x" ]; then
	cert_path=$default_cert_path;
fi

for f in $cert_path; do
	if [ `openssl x509 -in $f -noout -enddate 2>&1 | grep -c 'unable to load certificate'` = "0" ]; then
		enddate="$(openssl x509 -in $f -noout -enddate)"
		domain=`sudo /usr/bin/openssl x509 -noout -in $f -subject | tail -1 | awk 'BEGIN { FS = "/" } ; { print $7 }' | sed 's/CN=\*.//'`
		mydomain=`hostname -f | awk 'BEGIN{OFS=FS="."} {print $(NF-2),$(NF-1),$NF}'`
		if [ "$domain" != "$mydomain" ]; then
		    err="${err:+$err, }$(basename $f) has invalid domain"
		fi
	fi
done

if [ ! -z "$err" ]; then
	ret=2
	msg="$err${msg:+, $msg}"
fi

if [ "$cert_domain" -gt 0 ]; then
	echo "${ret:-0};${msg:-Ok}"
else
	echo "0;Ok, skipped"
fi
