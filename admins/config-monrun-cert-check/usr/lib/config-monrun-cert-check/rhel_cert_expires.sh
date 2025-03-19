#!/bin/bash
#
# Provides: mulca_cert_expires

me=${0##*/}     # strip path
me=${me%.*}     # strip extension

die() {
        echo "PASSIVE-CHECK:$me;$1;$2"
        exit 0
}

warndays=30
errdays=7

warn=
err=

cert_path=$1

if [ x"$cert_path" = "x" ]; then
	cert_path=/etc/yamail/ssl/*.crt;
fi

for f in $cert_path; do
	if [ `sudo /usr/bin/openssl x509 -noout -in $f -enddate 2>&1 | grep -c 'unable to load certificate'` = "0" ]; then
		enddate=`sudo /usr/bin/openssl x509 -noout -in $f -enddate`
		if ! sudo /usr/bin/openssl x509 -noout -in $f -checkend $(($warndays*86400)); then
			warn="${warn:+$warn, }$(basename $f) expires ${enddate#*=}"
		fi
		if ! sudo /usr/bin/openssl x509 -noout -in $f -checkend $(($errdays*86400)); then
			err="${err:+$err, }$(basename $f) expires ${enddate#*=}"
		fi
	fi
done

# Get Domain from installed certificate
domain=`sudo /usr/bin/openssl x509 -noout -in /etc/yamail/ssl/disk.yandex.ru.crt -subject | tail -1 | awk 'BEGIN { FS = "/" } ; { print $7 }' | sed 's/CN=\*.//'`
# Get host domain
mydomain=`hostname -f | awk 'BEGIN {FS="."} ; {print $2"."$3"."$4}'`

if [ "$domain" != "$mydomain" ]; then
    die "2" "Host domain differ from Disk certificate domain: $domain";
fi

if [ ! -z "$err" ]; then
	die "2" "$err";
fi

if [ ! -z "$warn" ]; then
	die "1" "$warn";
fi

die "0" "OK";
