MAILTO=''
{% if salt['pillar.get']('data:s3meta:update_buckets_size:enable', True) %}
{% set db = salt['pillar.get']('data:s3meta:update_buckets_size:db', salt['pillar.get']('data:s3meta:db')) %}
{% set user = salt['pillar.get']('data:s3meta:update_buckets_size:user', salt['pillar.get']('data:s3meta:user')) %}
{% set pgmeta = salt['pillar.get']('data:s3meta:update_buckets_size:pgmeta', salt['pillar.get']('data:s3pgmeta')) %}
0 1 * * 1 s3 flock -n /tmp/update_buckets_size.lock /usr/local/yandex/s3/s3meta/update_buckets_size/update_buckets_size.py -u '{{ user }}' -p '{{ pgmeta }}' -d '{{ db }}' >> /var/log/s3/update_buckets_size.log 2>&1
{%- endif %}
