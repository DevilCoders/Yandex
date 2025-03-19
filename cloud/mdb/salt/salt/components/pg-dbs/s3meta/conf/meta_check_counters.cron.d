MAILTO=''
{% set db = salt['pillar.get']('data:s3meta:db') %}
{% set user = salt['pillar.get']('data:s3meta:user') %}
{% set pgmeta = salt['pillar.get']('data:s3pgmeta') %}
30 6 * * * s3 sleep $[$RANDOM \% 150]; flock -n /tmp/check_chunks_counters_meta.lock /usr/local/yandex/s3/s3meta/check_chunks_bounds/check_chunks_bounds.py -p '{{ pgmeta }}' -d '{{ db }}' -u '{{ user }}' --critical-errors-filepath /var/log/s3/meta_critical_errors.log --run-on-replica >> /var/log/s3/meta_check_chunks_counters.log 2>&1
