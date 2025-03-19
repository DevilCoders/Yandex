#!/bin/bash

USAGE=$(cat <<"EOF"
Usage: valid_traffic.sh LOG TIMETOTAIL LOGTYPE INCLUDE1 EXCLUDE INCLUDE2 COMP_OP LIMIT ERRCODE\n
\tLOG   -   target logfile\n
\tTIMETOTAIL    -   seconds to look, timetail -n opt)\n
\tLOGTYPE   -   tymestamp type, timetail -t opt)\n
\tINCLUDE1  -   find this, usually virthost\n
\tEXCLUDE   -   exclude this, usually check urls like /ping\n
\tINCLUDE2  -   include finally, usually answer codes\n
\tCOMP_OP   -   operation type, like '-gt'\n
\tLIMIT     -   number of found lines for 'ok' result\n
\tERRCODE   -   code for 'err' result\n
EOF
)

if [ $# -ne 9 ]; then
    echo -en ${USAGE}
    exit 1
fi

LOG=$1
LOG2=`ls -t ${LOG}* | grep "\-[0-9]*\-[0-9]*$" | head -n1`
TIMETOTAIL=$2
LOGTYPE=$3
INCLUDE1=$4
EXCLUDE=$5
INCLUDE2=$6
COMP_OP=$7
LIMIT=$8
ERRCODE=$9


#/usr/bin/timetail -n ${TIMETOTAIL} -t ${LOGTYPE} $LOG $LOG2 2>/dev/null | grep -E "${INCLUDE1}" | grep -vE "${EXCLUDE}" | grep -E "${INCLUDE2}"

count=$(/usr/bin/timetail -n ${TIMETOTAIL} -t ${LOGTYPE} $LOG $LOG2 2>/dev/null | grep -E "${INCLUDE1}" | grep -vE "${EXCLUDE}" | grep -E "${INCLUDE2}" | wc -l)

if [ ! $count ];then
 count=0
fi

if [ $count `echo ${COMP_OP}` ${LIMIT} ]; then
  echo "0;ok"
else
  echo "${ERRCODE};err"
fi
