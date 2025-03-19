#!/usr/bin/env bash

LOGS=/var/log/mdb-maintenance
{% if salt['pillar.get']('data:mdb-maintenance:ca_dir') %}
export SSL_CERT_DIR={{ salt['pillar.get']('data:mdb-maintenance:ca_dir') }}
{% endif %}

{% if not salt['pillar.get']('data:mdb-maintenance:disable-maintenance-sync', False) %}
/opt/yandex/mdb-maintenance/mdb-maintenance-sync --config-path=/etc/yandex/mdb-maintenance/ >> $LOGS/mdb-maintenance-sync.log 2>> $LOGS/mdb-maintenance-sync.log
echo "$?" > /var/run/mdb-maintenance-sync/last-exit-status
{% else %}
echo "0" > /var/run/mdb-maintenance-sync/last-exit-status
{% endif %}
