MAILTO=''
{% set db = salt['pillar.get']('data:s3meta:chunk_mover:db', salt['pillar.get']('data:s3meta:db')) %}
{% set user = salt['pillar.get']('data:s3meta:chunk_mover:user', salt['pillar.get']('data:s3meta:user')) %}
{% set sentry_dsn = salt['pillar.get']('data:s3meta:sentry_dsn') %}
{% set one_shard_threshold = salt['pillar.get']('data:s3meta:chunk_mover:one_shard_threshold') %}
{% set koeff_shards_min = salt['pillar.get']('data:s3meta:chunk_mover:koeff_shards_min') %}
{% set koeff_shards_max = salt['pillar.get']('data:s3meta:chunk_mover:koeff_shards_max') %}
{% if salt['pillar.get']('data:s3meta:smart_mover:enable', True) %}
15 * * * * s3 flock -n /tmp/smart_mover.lock /usr/local/yandex/s3/s3meta/smart_mover/smart_mover.py -d '{{ db }}' -u {{ user }} {% if koeff_shards_min %} --koeff-shards-min {{ koeff_shards_min }}{% endif %}{% if koeff_shards_max %} --koeff-shards-max {{ koeff_shards_max }}{% endif %}{% if sentry_dsn %} --sentry-dsn '{{ sentry_dsn }}'{% endif %} >> /var/log/s3/smart_mover.log 2>&1
{% endif %}
