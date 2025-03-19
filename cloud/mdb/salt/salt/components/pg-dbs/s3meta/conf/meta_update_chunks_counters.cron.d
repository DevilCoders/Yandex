MAILTO=''
{% if salt['pillar.get']('data:s3meta:update_chunks_counters:enable', True) %}
{% set db = salt['pillar.get']('data:s3meta:update_chunks_counters:db', salt['pillar.get']('data:s3meta:db')) %}
{% set user = salt['pillar.get']('data:s3meta:update_chunks_counters:user', salt['pillar.get']('data:s3meta:user')) %}
{% set pgmeta = salt['pillar.get']('data:s3meta:update_chunks_counters:pgmeta', salt['pillar.get']('data:s3pgmeta')) %}
{% set threads = salt['pillar.get']('data:s3meta:update_chunks_counters:threads') %}
{% set sentry_dsn = salt['pillar.get']('data:s3meta:sentry_dsn') %}
* * * * * s3 flock -n /tmp/meta_update_chunks_counters.lock /usr/local/yandex/s3/s3meta/update_chunks_counters/update_chunks_counters.py -u '{{ user }}' -p '{{ pgmeta }}' -d '{{ db }}' {% if threads %}--threads={{ threads }}{% endif %} {% if sentry_dsn %}--sentry-dsn '{{ sentry_dsn }}'{% endif %} >> /var/log/s3/meta_update_chunks_counters.log 2>&1
{%- endif %}
