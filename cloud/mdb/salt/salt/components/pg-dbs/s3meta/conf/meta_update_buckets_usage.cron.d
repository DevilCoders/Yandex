MAILTO=''
{% if salt['pillar.get']('data:s3meta:update_buckets_usage:enable', True) %}
{% set db = salt['pillar.get']('data:s3meta:update_chunks_counters:db', salt['pillar.get']('data:s3meta:db')) %}
{% set user = salt['pillar.get']('data:s3meta:update_chunks_counters:user', salt['pillar.get']('data:s3meta:user')) %}
{% set pgmeta = salt['pillar.get']('data:s3meta:update_chunks_counters:pgmeta', salt['pillar.get']('data:s3pgmeta')) %}
{% set sentry_dsn = salt['pillar.get']('data:s3meta:sentry_dsn') %}
*/5 * * * * s3 flock -n /tmp/meta_update_buckets_usage.lock /usr/local/yandex/s3/s3meta/update_buckets_usage/update_buckets_usage.py -u '{{ user }}' -p '{{ pgmeta }}' -d '{{ db }}'{% if sentry_dsn %} --sentry-dsn '{{ sentry_dsn }}'{% endif %} >> /var/log/s3/meta_update_buckets_usage.log 2>&1
{%- endif %}
