{% from "components/postgres/pg.jinja" import pg with context %}
{# copypasted from salt/components/postgres/resize.sls #}
#!/bin/bash

set -x
export SYSTEMD_PAGER=''

sudo -u postgres /usr/local/yandex/pg_wait_started.py \
    -w {{ salt['pillar.get']('data:pgsync:recovery_timeout', '1200') }} \
    -m {{ salt['pillar.get']('data:versions:postgres:major_version') }} \
    || exit 1
sudo -u postgres /usr/local/yandex/pg_wait_synced.py -w ${TIMEOUT:-600} || exit 1

if ! service {{ pg.connection_pooler }} status
then
    service {{ pg.connection_pooler }} start
fi
