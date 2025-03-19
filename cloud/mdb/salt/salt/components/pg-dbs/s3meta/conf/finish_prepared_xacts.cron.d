MAILTO=''
{% set pgmeta = salt['pillar.get']('data:s3meta:finish_prepared_xacts:pgmeta', salt['pillar.get']('data:s3pgmeta')) %}
{% set db = salt['pillar.get']('data:s3meta:finish_prepared_xacts:db', salt['pillar.get']('data:s3meta:db')) %}
{% set user = salt['pillar.get']('data:s3meta:finish_prepared_xacts:user', salt['pillar.get']('data:s3meta:user')) %}
{% set delay = salt['pillar.get']('data:s3meta:finish_prepared_xacts:delay') %}
{% set sentry_dsn = salt['pillar.get']('data:s3meta:sentry_dsn') %}
{% if salt['pillar.get']('data:s3meta:finish_prepared_xacts:enable', True) %}
*/10 * * * * s3 flock -n /tmp/finish_prepared_xacts.lock /usr/local/yandex/s3/s3meta/finish_prepared_xacts/finish_prepared_xacts.py -p '{{ pgmeta }}' -d '{{ db }}' -u {{ user }} {% if delay %}--delay='{{ delay }}'{% endif %} {% if sentry_dsn %}--sentry-dsn '{{ sentry_dsn }}'{% endif %} >> /var/log/s3/finish_prepared_xacts.log 2>&1
{% endif %}
