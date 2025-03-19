#!/bin/bash
set -e

SA_ID=${1?}
YAV_SECRET_ID=${2?}

#openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:4096 -out ${SA_ID?}.key
#openssl pkey -in ${SA_ID?}.key -pubout -out ${SA_ID?}.pub
#SA_KEY_PUB=$(awk '{printf "%s\\n", $0}' ${SA_ID?}.pub)
#yav create version ${YAV_SECRET_ID?} -u -f ${SA_ID?}.key=${SA_ID?}.key ${SA_ID?}.pub=${SA_ID?}.pub > /dev/null

yav get version ${YAV_SECRET_ID?} -O ${SA_ID?}.pub | awk '{printf "%s\\n",$0}'
