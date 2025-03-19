MAILTO=''
{% set pgmeta = salt['pillar.get']('data:s3db:chunk_splitter:pgmeta', salt['pillar.get']('data:s3pgmeta')) %}
{% set db = salt['pillar.get']('data:s3db:chunk_splitter:db', salt['pillar.get']('data:s3db:db', 'host=localhost port=6432 dbname=s3db user=s3util')) %}
{% set threshold = salt['pillar.get']('data:s3db:chunk_splitter:threshold') %}
{% set user = salt['pillar.get']('data:s3db:chunk_splitter:user', salt['pillar.get']('data:s3db:user', 's3util')) %}
{% set sentry_dsn = salt['pillar.get']('data:s3db:sentry_dsn') %}
{% if salt['pillar.get']('data:s3db:chunk_splitter:enable', True) %}
*/5 * * * * s3 sleep $[$RANDOM \% 150]; flock -n /tmp/chunk_splitter.lock /usr/local/yandex/s3/s3db/chunk_splitter/chunk_splitter.py -p '{{ pgmeta }}' -d '{{ db }}' -u {{ user }} {% if threshold %}-t '{{ threshold }}'{% endif %} {% if sentry_dsn %}--sentry-dsn '{{ sentry_dsn }}'{% endif %} >> /var/log/s3/chunk_splitter.log 2>&1
{% endif %}
