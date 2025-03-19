MAILTO=''
{% set pgmeta = salt['pillar.get']('data:s3meta:chunk_mover:pgmeta', salt['pillar.get']('data:s3pgmeta')) %}
{% set db = salt['pillar.get']('data:s3meta:chunk_mover:db', salt['pillar.get']('data:s3meta:db')) %}
{% set user = salt['pillar.get']('data:s3meta:chunk_mover:user', salt['pillar.get']('data:s3meta:user')) %}
{% set threshold = salt['pillar.get']('data:s3meta:chunk_mover:threshold') %}
{% set max_objects = salt['pillar.get']('data:s3meta:chunk_mover:max_objects') %}
{% set min_objects = salt['pillar.get']('data:s3meta:chunk_mover:min_objects') %}
{% set delay = salt['pillar.get']('data:s3meta:chunk_mover:delay') %}
{% set max_threads = salt['pillar.get']('data:s3meta:chunk_mover:max_threads') %}
{% set allow_same_shard = salt['pillar.get']('data:s3meta:chunk_mover:allow_same_shard') %}
{% set sentry_dsn = salt['pillar.get']('data:s3meta:sentry_dsn') %}
{% if salt['pillar.get']('data:s3meta:chunk_mover:enable', True) %}
* * * * * s3 flock -n /tmp/chunk_mover.lock /usr/local/yandex/s3/s3meta/chunk_mover/chunk_mover.py -p '{{ pgmeta }}' -d '{{ db }}' -u {{ user }} -q {% if max_threads %}--max-threads={{ max_threads }}{% endif %} {% if allow_same_shard %}--allow-same-shard{% endif %} {% if sentry_dsn %}--sentry-dsn '{{ sentry_dsn }}'{% endif %} >> /var/log/s3/chunk_mover.log 2>&1
{% if salt['pillar.get']('data:s3meta:chunk_mover:auto', False) %}
*/3 * * * * s3 sleep $[$RANDOM \% 600]; flock -n /tmp/chunk_mover.lock /usr/local/yandex/s3/s3meta/chunk_mover/chunk_mover.py -p '{{ pgmeta }}' -d '{{ db }}' -u {{ user }} {% if threshold %}--diff-threshold={{ threshold }}{% endif %} {% if max_objects %}--max-objects={{ max_objects }}{% endif %} {% if min_objects %}--min-objects={{ min_objects }}{% endif %} {% if delay %}--delay='{{ delay }}'{% endif %} {% if sentry_dsn %}--sentry-dsn '{{ sentry_dsn }}'{% endif %} >> /var/log/s3/chunk_mover.log 2>&1
{% endif %}
{% endif %}
