#!/bin/bash

if [[ -f run_local.sh ]]; then
    # has debug/testing runner - use it
    . run_local.sh
    exit
fi

if [[ -f run_include.sh ]]; then
    # has include runner - use it
    . run_include.sh
fi

# set defaults

if [[ -z "$TTS_LOG_LEVEL" ]]; then
    TTS_LOG_LEVEL=WARNING
fi

if [[ -z "$TTS_USE_CUDA" ]]; then
    TTS_USE_CUDA=1
fi

if [[ "$TTS_USE_CUDA" != "1" ]]; then
    export CUDA_VISIBLE_DEVICES=-1
fi

if [[ -z "$TTS_MAXCONNECTION" ]]; then
    TTS_MAXCONNECTION=12
fi

if [[ -z "$TTS_TOTALCONNECTION" ]]; then
    TTS_TOTALCONNECTION=`echo $TTS_MAXCONNECTION | awk {'printf ("%.0f\n"),($1*1.2+10)'}`
fi

CFGFILE=tts-server.xml
echo """
<?xml version="1.0" encoding="UTF-8"?>
<config>
    <serverconfig>
        <port>17004</port>
        <accesslog>STDOUT</accesslog>
        <loglevel>$TTS_LOG_LEVEL</loglevel>
        <cpuLimit>false</cpuLimit>
        <chunkedtimeout>7</chunkedtimeout>
        <rtlog_file_name>/logs/tts.eventlog</rtlog_file_name>
        <rtlog_service_name>tts-server-gpu</rtlog_service_name>
        <gpu_required>$TTS_USE_CUDA</gpu_required>
        <ttslang>$TTS_LANG</ttslang>
        <maxconnection>$TTS_MAXCONNECTION</maxconnection>
        <totalconnection>$TTS_TOTALCONNECTION</totalconnection>""" >$CFGFILE

if [[ -e lingware/ ]]; then
    LW=`ls lingware/ --hide=_iss*`
    echo "        <ttsconfig>lingware/$LW/</ttsconfig>" >>$CFGFILE
else
    >&2 echo "Lingware does not exist"
    exit 1;
fi

echo """    </serverconfig>
</config>
""" >>$CFGFILE

echo "##### BEGIN $CFGFILE #####"
cat $CFGFILE
echo "##### END $CFGFILE #####"

# set TF_CUDNN_.. for sgalustyan@
export TF_CUDNN_USE_AUTOTUNE=0

# limit extra threads
export OMP_NUM_THREADS=1

echo "##### RUN tts-server #####"
exec ./tts-server $CFGFILE
