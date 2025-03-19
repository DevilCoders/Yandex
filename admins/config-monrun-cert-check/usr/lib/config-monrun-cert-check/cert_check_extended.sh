#!/bin/bash

usage() {
	echo "Usage:
	$0 -h HOST -p PORT -e EXPIRE_DAYS
or
	$0 -h HOST -p PORT -i ISSUER"
	exit 1
}

while getopts ":h:p:e:i:" opt ; do
	case "$opt" in
		h)
			HOST=$OPTARG
			;;
		p)
			PORT=$OPTARG
			;;
		e)
			EXPIRE_DAYS=$OPTARG
			;;
		i)
			ISSUER=$OPTARG
			;;
		?)
			usage
			;;
	esac
done


if [ ! -z "$ISSUER" ] && [ ! -z $EXPIRE_DAYS ] ; then
	usage
fi

cert_file=$(mktemp)

trap "[ -f $cert_file ] && rm -f $cert_file" EXIT

openssl s_client -connect ${HOST}:${PORT} < /dev/null 2>&1 | sed -n '/BEGIN CERT/,/END CERT/p ' > $cert_file
if [ ! -s $cert_file ] ; then
	echo "1; Failed to establish SSL connection with $HOST:$PORT"
	exit 0
fi

if [ ! -z "$ISSUER" ] ; then
	current_issuer=$(openssl  x509 -in $cert_file -noout -issuer 2>&1 | awk 'BEGIN{RS="/";FS="="}{if($1~/^CN/){print $2}}')
	if $(echo $current_issuer | grep -qi "$ISSUER"); then
		echo "0; Ok, certificate issuer $current_issuer"
	else
		echo "2; Failed, certificate issuer $current_issuer"
	fi
	exit 0
fi


if [ ! -z $EXPIRE_DAYS ] ; then
	if openssl  x509 -in $cert_file -noout -checkend $((86400*EXPIRE_DAYS)) > /dev/null 2>&1; then
		echo "0; Ok, certificate is still valid"
	else
		echo "2; Failed, certificate will expire soon"
	fi
	exit 0
fi
