MAILTO=''
{% set pgmeta = salt['pillar.get']('data:s3db:chunk_merger:pgmeta', salt['pillar.get']('data:s3pgmeta')) %}
{% set db = salt['pillar.get']('data:s3db:chunk_merger:db', salt['pillar.get']('data:s3db:db', 'host=localhost port=6432 dbname=s3db user=s3util')) %}
{% set user = salt['pillar.get']('data:s3db:chunk_merger:user', salt['pillar.get']('data:s3db:user', 's3util')) %}
{% set exclude_bids = salt['pillar.get']('data:s3db:chunk_merger:exclude_bids') %}
{% set sentry_dsn = salt['pillar.get']('data:s3db:sentry_dsn') %}
{% if salt['pillar.get']('data:s3db:chunk_merger:enable', True) %}
*/5 * * * * s3 sleep $[$RANDOM \% 150]; flock -n /tmp/chunk_merger.lock /usr/local/yandex/s3/s3db/merge_chunks/merge_chunks.py -p '{{ pgmeta }}' -d '{{ db }}' -u {{ user }}{% if exclude_bids %} --exclude-bids '{{ exclude_bids }}'{% endif %}{% if sentry_dsn %} --sentry-dsn '{{ sentry_dsn }}'{% endif %} >> /var/log/s3/chunk_merger.log 2>&1
{% endif %}
