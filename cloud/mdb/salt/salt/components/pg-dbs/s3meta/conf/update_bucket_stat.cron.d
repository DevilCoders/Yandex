MAILTO=''
{% if salt['pillar.get']('data:s3meta:update_bucket_stat:enable', True) %}
{% set db = salt['pillar.get']('data:s3meta:update_bucket_stat:db', salt['pillar.get']('data:s3meta:db')) %}
* * * * * s3 flock -n /tmp/update_bucket_stat.lock /usr/local/yandex/s3/s3meta/update_bucket_stat/update_bucket_stat.py -d '{{ db }}' >> /var/log/s3/update_bucket_stat.log 2>&1
{%- endif %}
