#!/usr/bin/env bash

MYDIR=`dirname "${BASH_SOURCE[0]}"`

RELEASE_TYPE=release
if [ $# -ge 1 ]; then
    RELEASE_TYPE=$1
fi

if [ ${RELEASE_TYPE} != "release" ] && [ ${RELEASE_TYPE} != "debug" ]; then
    echo "Unknown release type <${RELEASE_TYPE}>"
    exit 1
fi


# check for compiler (older gcc can not build our cython modules)
if [ -z ${CC} ]; then
    COMPILER_FOUND=0
    for COMPILER in gcc gcc-4.7; do
        COMPILER=`which ${COMPILER}`
        if [ $? -ne 0 ]; then
            continue
        fi

        VERSION=`${COMPILER} -dumpversion | sed -e 's/\.\([0-9][0-9]\)/\1/g' -e 's/\.\([0-9]\)/0\1/g' -e 's/^[0-9]\{3,4\}$/&00/'`

        if (( ${VERSION} == 40700 )); then
            CC=`which ${COMPILER}`
            COMPILER_FOUND=1
        fi

    done

    if (( ${COMPILER_FOUND} == 0 )); then
        echo "All found compilers are older than 40700, exiting ..."
        exit 1
    fi
fi

# clean old stuff
rm -f ${MYDIR}/core/hosts.so ${MYDIR}/core/instances.so ${MYDIR}/core/ghi.so ${MYDIR}/core/intlookups.so

# run build
if [ ${RELEASE_TYPE} == "debug" ]; then
CYTHON_DEBUG=1 CC=${CC} LINKCC=${CC} /skynet/python/bin/python ${MYDIR}/build_cython.py build_ext --inplace || exit 1
else
CC=${CC} LINKCC=${CC} /skynet/python/bin/python ${MYDIR}/build_cython.py build_ext --inplace || exit 1
fi

mv ${MYDIR}/hosts.so ${MYDIR}/instances.so ${MYDIR}/ghi.so ${MYDIR}/intlookups.so ${MYDIR}/core || exit 1
