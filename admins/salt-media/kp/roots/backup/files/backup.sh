#!/usr/bin/env bash

ulimit -c unlimited

pushd $(dirname $0) > /dev/null
ROOT_PATH=$(pwd -P)
popd > /dev/null

if [ $# -ne 1 ]; then
    echo -e "USAGE:\t$0 HOSTNAME\n\t ${ROOT_PATH}/conf.d/HOSTNAME.include file must be present"
    exit 1
fi

HOST=$1
DATE=$(date +%F)
DATA_PATH=${ROOT_PATH}/backups
CONF_PATH=${ROOT_PATH}/conf.d
LOG_PATH=${ROOT_PATH}/logs
LOG_FILE=${LOG_PATH}/${HOST}.log

mkdir -p ${CONF_PATH}
mkdir -p ${LOG_PATH}

CONF_INCLUDE=${CONF_PATH}/${HOST}.include
CONF_EXCLUDE=${CONF_PATH}/${HOST}.exclude

BACKUP_ROOT=${DATA_PATH}/${HOST}
BACKUP_FULL=${BACKUP_ROOT}/full
BACKUP_INCREMENTAL_DIR=${BACKUP_ROOT}/incremental/${DATE}

OPTS="--force --delete --backup --backup-dir=${BACKUP_INCREMENTAL_DIR} --ignore-errors -av"
LAST=$(cat ${BACKUP_ROOT}/.result 2>/dev/null |grep last_backup| grep -oP '\d{4}-\d{2}-\d{2}')

if [ ! -f "${CONF_INCLUDE}" ]; then
    echo "ERROR: There is no include-file for ${HOST}" >> ${LOG_FILE}
    exit 1;
fi

if [ "${LAST}X" = "X" ]; then
    echo "WARNING: There is no last backup for ${HOST}" >> ${LOG_FILE}
fi

if [ "${LAST}" = "${DATE}" ]; then
    echo "ERROR: Last backup already done for ${HOST}: \"${LAST}\"" >> ${LOG_FILE}
    exit 1;
fi


mkdir -p ${BACKUP_FULL}
mkdir -p ${BACKUP_INCREMENTAL_DIR}

EXCLUDE=''
[ -f ${CONF_EXCLUDE} ] && EXCLUDE="--exclude-from=${CONF_EXCLUDE}"

ERR_COUNT=0
for path in $(cat ${CONF_INCLUDE} |grep -v '^#\|^$'); do
    rsync ${OPTS} ${EXCLUDE} -e ssh ${HOST}:${path} ${BACKUP_FULL}/ >> ${LOG_FILE}
    if [ $? -eq 0 ]; then
            echo "${DATE} backup of ${HOST}:${path} succeeded!" >> ${LOG_FILE}
    else
            let ERR_COUNT++
            echo "${DATE} backup of ${HOST}:${path} failed!" >> ${LOG_FILE}
    fi
done

if [ ${ERR_COUNT} -eq 0 ]; then
    tar -cf $BACKUP_INCREMENTAL_DIR.tar.gz -I pigz --remove-files $BACKUP_INCREMENTAL_DIR >> ${LOG_FILE} 2>&1
    echo "last_backup: ${DATE}" > ${BACKUP_ROOT}/.result
else
    echo "last_error: ${DATE}" > ${BACKUP_ROOT}/.result
fi
