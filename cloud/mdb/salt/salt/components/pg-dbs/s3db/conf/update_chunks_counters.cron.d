MAILTO=''
{% if salt['pillar.get']('data:s3db:update_chunks_counters:enable', True) %}
{% set db = salt['pillar.get']('data:s3db:update_chunks_counters:db', salt['pillar.get']('data:s3db:db', 'host=localhost port=6432 dbname=s3db user=s3util')) %}
{% set batch = salt['pillar.get']('data:s3db:update_chunks_counters:batch_size') %}
{% set sentry_dsn = salt['pillar.get']('data:s3db:sentry_dsn') %}
* * * * * s3 sleep $[$RANDOM \% 30]; flock -n /tmp/update_chunks_counters.lock /usr/bin/timeout 3600 /usr/local/yandex/s3/s3db/update_chunks_counters/update_chunks_counters.py -d '{{ db }}' {% if batch %}--batch {{ batch }}{% endif %} {% if sentry_dsn %}--sentry-dsn '{{ sentry_dsn }}'{% endif %} >> /var/log/s3/update_chunks_counters.log 2>&1
{%- endif %}
