MAILTO=''
{% set pgmeta = salt['pillar.get']('data:s3meta:chunk_purger:pgmeta', salt['pillar.get']('data:s3pgmeta')) %}
{% set db = salt['pillar.get']('data:s3meta:chunk_purger:db', salt['pillar.get']('data:s3meta:db')) %}
{% set user = salt['pillar.get']('data:s3meta:chunk_purger:user', salt['pillar.get']('data:s3meta:user')) %}
* * * * * s3 flock -n /tmp/chunk_purger.lock /usr/local/yandex/s3/s3meta/chunk_purger/chunk_purger.py -p '{{ pgmeta }}' -d '{{ db }}' -u {{ user }} >> /var/log/s3/chunk_purger.log 2>&1
