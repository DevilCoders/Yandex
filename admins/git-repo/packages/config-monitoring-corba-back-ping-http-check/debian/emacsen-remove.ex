#!/bin/sh -e
# /usr/lib/emacsen-common/packages/remove/config-monitoring-corba-back-ping-http-check

FLAVOR=$1
PACKAGE=config-monitoring-corba-back-ping-http-check

if [ ${FLAVOR} != emacs ]; then
    if test -x /usr/sbin/install-info-altdir; then
        echo remove/${PACKAGE}: removing Info links for ${FLAVOR}
        install-info-altdir --quiet --remove --dirname=${FLAVOR} /usr/share/info/config-monitoring-corba-back-ping-http-check.info.gz
    fi

    echo remove/${PACKAGE}: purging byte-compiled files for ${FLAVOR}
    rm -rf /usr/share/${FLAVOR}/site-lisp/${PACKAGE}
fi
