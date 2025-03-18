#!/usr/local/bin/bash

ARCADIA_PATH="../../../.."
# Папка с базой
PATH_TO_BASE=$1
# Файл с url, по одному url на строку
URL_LIST=$2
# Список найденных пар url "\t" docId будет сохраняться сюда
DOC_ID_LIST=$3

if [ ! -d $ARCADIA_PATH/tools/printurls ]
then
    if [ ! -f $ARCADIA_PATH/tools/printurls/printurls ]
    then
        om $ARCADIA_PATH/tools/printurls
    fi
fi

$ARCADIA_PATH/tools/printurls/printurls -a -n $PATH_TO_BASE/index < $URL_LIST > $DOC_ID_LIST

