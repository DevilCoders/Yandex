#!/usr/bin/env bash

MYDIR=`dirname "${BASH_SOURCE[0]}"`

rm -rf ${MYDIR}/virtualenv
mkdir ${MYDIR}/virtualenv

VENVPATH=`realpath ${MYDIR}/virtualenv`

/usr/local/bin/virtualenv ${VENVPATH}

source virtualenv/bin/activate
virtualenv/bin/python virtualenv/bin/pip install -r ${MYDIR}/requirements.txt
deactivate
