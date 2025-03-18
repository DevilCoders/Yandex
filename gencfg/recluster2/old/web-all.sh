#!/usr/bin/env bash

SDIR=`dirname "${BASH_SOURCE[0]}"`

source ${SDIR}/executor.sh
source ${SDIR}/../scripts/run.sh

# cleanup all
function cleanup() {
    run ${SDIR}/web-priemka.sh start
    run ${SDIR}/web-r1.sh start
    run ${SDIR}/web-prod-msk.sh start
    run ${SDIR}/web-prod-sas.sh start
    run ${SDIR}/web-prod-man.sh start
}

# reallocate hosts
# order is so important
function allocate_hosts() {
    run ${SDIR}/web-priemka.sh mid
    run ${SDIR}/web-r1.sh mid
    run ${SDIR}/web-prod-msk.sh mid
    run ${SDIR}/web-prod-sas.sh mid
    run ${SDIR}/web-prod-man.sh mid
}

# recluster all
function recluster() {
    run ${SDIR}/web-priemka.sh end
    run ${SDIR}/web-r1.sh end
    run ${SDIR}/web-prod-msk.sh end
    run ${SDIR}/web-prod-sas.sh end
    run ${SDIR}/web-prod-man.sh end
}

select_action cleanup allocate_hosts recluster
