#!/bin/sh -e
# finds subnets with many requests
# input format: <num requests> <ip>

SCRIPTDIR=`dirname $0`;

awk '{if ($1 > 100) printf("%s\t%s\n", $2, $1)}' | ${SCRIPTDIR}/../filter_yandex.py | sort | ${SCRIPTDIR}/find_subnets.py | sort -nr -k 5;

