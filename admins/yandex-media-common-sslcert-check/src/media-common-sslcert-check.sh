#!/bin/bash

if [[ ${#} -ne 2 ]]; then
    echo "2;usage: ${0} {SERVER_NAME} {EXPIRE_SECONDS}"
    exit 0
fi

servername="${1}"
seconds="${2}"

cert=$(gnutls-cli --print-cert "${servername}" \
                   </dev/null 2>/dev/null)
ret=$?

if [[ $ret -ne 0 ]]; then
    echo "2;can't get ssl certificate for ${servername}"
    exit 0
fi

exp=$(echo "${cert}" | openssl x509 -enddate -noout)

echo "${cert}" | openssl x509 -checkend "${seconds}" >/dev/null 2>&1
ret=$?

if [[ $ret -ne 0 ]]; then
    echo "2;plaese check cert expiration; ${exp}"
else
    echo "0;cert is ok; $exp"
fi
