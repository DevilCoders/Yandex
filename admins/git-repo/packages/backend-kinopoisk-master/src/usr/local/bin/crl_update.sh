#!/usr/bin/env bash

usage(){
    echo "Usage: $0 <-t time>"
    echo "    -t <time>   Set time gap (between exp. date and reissueance)"
    echo "                in seconds. Default value is 172800 (48 hours)."
    echo "    -f          Update now (Force mode)."
}

ARGS=`getopt -o ht:f --long help,debug -- "$@"`
eval set -- "$ARGS"

while true ; do
    case "$1" in
        -f) force=1; shift;;
        -t) time_gap=$2; shift 2;;
        -h|--help) usage; exit 0;;
        --debug) set -x; shift;;
        --) shift; break;;
        *) echo "Internal error!"; usage; exit 1;;
    esac
done
                                                                                                                                                       
CRL_CERT="/home/ca/rootCA/crl.crt"                                                                                                                    
CA_CONFIG="/home/ca/rootCA/ca.conf"                                                                                                                   
CA_PASS="/home/ca/rootCA/pass"                                                                                                                        

# Time gap in seconds in which new cert should be issued
# before the previous one become expired.
time_gap=${time_gap:-172800} # Default value Two days

# Dates in seconds
current_date=`date +'%s'`
crl_exp_date=`date +'%s' \
        -d "\`openssl crl -in $CRL_CERT -text -noout \
            |grep "Next Update" \
            |sed -e  's/\s\{1,\}Next Update:\s//'\`"`


# Check whether we need to reissue new CRL cert.
if [[ $((crl_exp_date - current_date)) -gt $time_gap ]]; then
    if [[ ! "$force" ]]; then
        # reIssuance dos not required
        exit 0
    fi
fi

# Issuance
cd `dirname $CA_CONFIG`
openssl ca \
        -gencrl \
        -config $CA_CONFIG \
        -out $CRL_CERT \
        -passin file:$CA_PASS \
        > /dev/null 2>&1
exit 0
