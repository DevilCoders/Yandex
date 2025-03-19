MAILTO=''
{% if salt['pillar.get']('data:s3meta:update_shard_stat:enable', True) %}
{% set db = salt['pillar.get']('data:s3meta:update_shard_stat:db', salt['pillar.get']('data:s3meta:db')) %}
* * * * * s3 flock -n /tmp/update_shard_stat.lock /usr/local/yandex/s3/s3meta/update_shard_stat/update_shard_stat.py -d '{{ db }}' >> /var/log/s3/update_shard_stat.log 2>&1
{%- endif %}
