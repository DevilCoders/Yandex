#!/bin/bash

internal="YandexInternalCA"

env_type=`cat /etc/yandex/environment.type 2>/dev/null`
env_name=`cat /etc/yandex/environment.name 2>/dev/null`

warn=
err=

cert_path=$1

# include default cert pathes
. /usr/lib/config-monrun-cert-check/cert_path.sh

if [ x"$cert_path" = "x" ]; then
	cert_path=$default_cert_path;
fi

for f in $cert_path; do
	if [ x"$env_type" = x"production" ] && [ x"$env_name" != x"intranet" ] &&
	   [ `openssl x509 -in $f -noout -enddate 2>&1 | grep -c 'unable to load certificate'` = "0" ]; then
		cn=`openssl x509 -in $f -noout -issuer | awk 'BEGIN{RS="/";FS="="}{if($1~/^CN/){printf($2)}}'`
		if [ x"$cn" = x"YandexInternalCA" ]; then
			err="${err:-"2;internal certs on production:"} $(basename $f)"
		fi
	fi
done

echo "${err:-0;Ok}"
