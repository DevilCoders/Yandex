{%- set config = mediastorage -%}
#!/bin/bash

set -o errtrace
set -o nounset

LOG_FILE=/var/log/mds/restart.log
NO_CHECK=false
WAIT_TIME=600
USE_FILE={{ config.use_file|lower }}

maintenance_file=/etc/nginx/maintenance.file
iptruler_status=$(iptruler all status | grep -o "opened" | wc -l)

function log() {
    echo "$(date +"%Y-%m-%d %H:%M:%S") ${@}" >> $LOG_FILE 2>&1
}

function log_n() {
    echo -n "${@}" >> $LOG_FILE 2>&1
}

function inf_loop() {
    while true; do
        sleep 100;
    done
}

function close_host() {
    if [[ $USE_FILE = true ]]; then
        if [[ -e $maintenance_file ]]; then
            log "Host was closed before restart"
        else
            log "Creating maintenance file /etc/nginx/maintenance.file"
            touch $maintenance_file && echo $(date +%s) > $maintenance_file
        fi
    else
        if [[ $iptruler_status -lt 2 ]]; then
            log "Host was closed before restart"
        else
            log "iptruler all down"
            iptruler all down
        fi
    fi
}


log "Start restarting"

if [[ $USE_FILE = true ]]; then
    log "Restart using maintenance file"
else
    log "Restart using iptruler"
fi
close_host

if [[ $USE_FILE = true ]]; then
    while [[ $(timetail -t tskv -n 5 /var/log/nginx/tskv.log | grep -Evc 'combaine|status=418|request=(/ping|/unistat|/get_storage_state_snapshot|/get_config_remotes|/get_cached_keys)') -gt 5 ]] ; do sleep 1 ; log_n "." ; done
    while [[ $(timetail -t tskv -n 5 /var/log/nginx/downloader/access.log | grep -Evc 'combaine|status=401') -gt 5 ]] ; do
        if [[ $(($(date +%s)-$(cat $maintenance_file))) -gt $WAIT_TIME ]]; then
            log "Can't wait more than $WAIT_TIME seconds"
            /etc/init.d/nginx stop
        else
            sleep 1
            log_n "."
        fi
    done
else
    while [[ $(timetail -t tskv -n 1 /var/log/nginx/tskv.log | grep -Evc 'request=(/ping|/unistat|/s3_unistat)|status=(403|404|410)') -gt 20 ]] ; do sleep 1 ; log_n "." ; done
    while [[ $(timetail -t tskv -n 1 /var/log/nginx/int-tskv.log | grep -Evc 'request=/ping|status=(403|404|410)') -gt 20 ]] ; do sleep 1 ; log_n "." ; done
    while [[ $(timetail -t tskv -n 1 /var/log/nginx/s3-tskv.log | grep -Evc 'request=.{1,}ping') -gt 20 ]] ; do sleep 1 ; log_n "." ; done
fi

echo $'\r' >> $LOG_FILE 2>&1
log "..done"

log "Stopping nginx"
/etc/init.d/nginx stop
while [[ $(pgrep -c nginx) != 0 ]] ; do sleep 0.5 ; log_n "."; done
log "..done"

log "Restarting mediastorage-proxy"
ubic restart mediastorage-proxy
log "Waiting for mediastorage-proxy up and running"
while true ; do ubic status mediastorage-proxy 2>&1 > /dev/null && log "..done" && break || sleep 1 ; log_n "." ; done
log "..done"

log "Starting nginx"
/etc/init.d/nginx start

# launch tests only for proxy and storage hosts, because disk has some troubles with secrets from yav
{% if config.run_tests %}
if [[ $USE_FILE = false ]]; then
    log "Test writing file"
    upload=$(curl -sg -d "$(date +%s)" "[::]:{{ config.port }}/upload-khranilishe/mds_proxy-restarter_$(hostname | awk -F'.' '{print $1}')" -w "%{http_code}")
    upload_code=$(echo $upload | awk -F"</post>" '{print $2}')
    if [[ $upload_code == 200 ]]; then
        upload_key=$(echo $upload | sed 's/\s/\n/g' | grep key | awk -F'"' '{print $2}')
        log "Key has been successfully written: $upload_key"
    elif [[ $upload_code == 403 ]]; then
        while [[ $upload_code == 403 ]]; do
            upload_key=$(echo $upload | sed 's/\s/\n/g'| grep key | sed 's/</>/g' | awk -F'>' '{print $3}')
            log "Can't upload file, previous hasn't been removed"
            log "Try to remove old file with key $upload_key"
            if [[ $(curl -g "[::]:10011/delete-khranilishe/$upload_key" -w %{http_code}) == '200' ]]; then
                log "Successfully deleted $upload_key"
                log "Try to upload file again"
                upload=$(curl -sg -d "$(date +%s)" "[::]:{{ config.port }}/upload-khranilishe/mds_proxy-restarter_$(hostname | awk -F'.' '{print $1}')" -w "%{http_code}")
                upload_code=$(echo $upload | awk -F"</post>" '{print $2}')
                upload_key=$(echo $upload | sed 's/\s/\n/g' | grep key | awk -F'"' '{print $2}')
            else
                log "Can't delete $upload_key, entering an infinite loop"
                inf_loop
            fi
        done
    else
        log "Can't upload the file, entering an infinite loop"
        inf_loop
    fi
    log "Test reading file"
    if [[ $(curl -g -H "X-MDS-SECURITY: {{ pillar['yav']['x_mds_security'] }}" "[::]:{{ config.port }}/get-khranilishe/$upload_key" -w "%{http_code}" -o /dev/null) != '200' ]]; then
        log "Can't read the file, entering an infinite loop"
        inf_loop
    else
        log "Key has been successfully read: $upload_key"
    fi
    log "Test removing file"
    if [[ $(curl -g "[::]:{{ config.port }}/delete-khranilishe/$upload_key" -w %{http_code}) != '200' ]]; then
        log "Can't remove the file, entering an infinite loop: $upload_key"
        inf_loop
    else
        log "Key has been successfully removed: $upload_key"
    fi
else
    log "Test reading file"
    {% set federation = pillar.get('mds_federation', 1) %}
    {% if federation == 1 -%}
    {% if grains['yandex-environment'] == 'testing' -%}
    upload_key="1397886/mds_proxy_read_restarter"
    {%- else -%}
    upload_key="1540651/mds_proxy_read_restarter"
    {%- endif %}
    log "Key: $upload_key"
    if [[ $(curl -g -H "X-MDS-SECURITY: {{ pillar['yav']['x_mds_security'] }}" "[::]:{{ config.port }}/get-khranilishe/$upload_key" -w "%{http_code}" -o /dev/null) != '200' ]]; then
        log "Can't read the file, entering an infinite loop"
        inf_loop
    else
        log "Key has been successfully read"
    fi
    {%- endif %}
fi
{% endif %}


if [[ $USE_FILE = true ]]; then
    if [[ ! $(cat $maintenance_file) ]] || [[ $(($(date +%s)-$(cat $maintenance_file))) -eq $(date +%s) ]]; then
        log "Host was already closed"
        NO_CHECK=true
    else
        log "Removing maintenance file"
        rm $maintenance_file
    fi
elif [[ $iptruler_status == '2' ]] ; then
	    log "iptruler all up"
	    iptruler all up
	    sleep 2
else
    log "Host was already closed"
    NO_CHECK=true
fi

if [[ $NO_CHECK = false ]]; then
    log "Checking for 5xx statuses"
    sleep 10
    if [[ $(timetail -t tskv -n 5 /var/log/nginx/tskv.log | grep -c ' status=5..') -gt 5 ]]; then
        log "Found 5xx response codes, closing host"
        close_host
    else
        log "It's fine, mediastorage-proxy successfully restarted"
    fi
fi

log "End restarting"
