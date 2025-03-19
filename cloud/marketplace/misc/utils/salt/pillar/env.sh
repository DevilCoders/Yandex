#!/bin/bash

base=$(dirname ${BASH_SOURCE[0]})

if [ -f $base/${ENV}/secret/env.sh ]
then source $base/${ENV}/secret/env.sh
fi

if [ -f $base/${ENV}/common/env.sh ]
then source $base/${ENV}/common/env.sh
fi
