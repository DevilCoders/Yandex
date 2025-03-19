MAILTO=''
{% set pgmeta = salt['pillar.get']('data:s3meta:chunk_creator:pgmeta', salt['pillar.get']('data:s3pgmeta')) %}
{% set db = salt['pillar.get']('data:s3meta:chunk_creator:db', salt['pillar.get']('data:s3meta:db')) %}
{% set user = salt['pillar.get']('data:s3meta:chunk_creator:user', salt['pillar.get']('data:s3meta:user')) %}
{% set sentry_dsn = salt['pillar.get']('data:s3meta:sentry_dsn') %}
* * * * * s3 flock -n /tmp/chunk_creator.lock /usr/local/yandex/s3/s3meta/chunk_creator/chunk_creator.py -p '{{ pgmeta }}' -d '{{ db }}' -u {{ user }} {% if sentry_dsn %}--sentry-dsn '{{ sentry_dsn }}'{% endif %} >> /var/log/s3/chunk_creator.log 2>&1
