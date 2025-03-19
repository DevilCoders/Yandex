#!/bin/sh

exec `dirname $0`/../../../../../../../kms/scripts/deploy/run-deploy.sh \
    --terraform-dir=`dirname $0`/.. \
    --deploy-spec=`dirname $0`/deploy.json \
    $*
