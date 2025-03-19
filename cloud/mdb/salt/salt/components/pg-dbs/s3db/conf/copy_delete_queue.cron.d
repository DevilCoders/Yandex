MAILTO=''
{% set db = salt['pillar.get']('data:s3db:copy_delete_queue:db', salt['pillar.get']('data:s3db:db', 'host=localhost port=6432 dbname=s3db user=s3util')) %}
{% set sentry_dsn = salt['pillar.get']('data:s3db:sentry_dsn') %}
{% set min_free_time = salt['pillar.get']('data:s3db:copy_delete_queue:min_free_time', '1 year') %}
{% if salt['pillar.get']('data:s3db:copy_delete_queue:enable', True) %}
*/10 * * * * s3 sleep $[$RANDOM \% 150]; flock -n /tmp/copy_delete_queue.lock /usr/local/yandex/s3/s3db/copy_delete_queue/copy_delete_queue.py -d '{{ db }}' {% if sentry_dsn %} --sentry-dsn '{{ sentry_dsn }}'{% endif %} -t {{ min_free_time }} >> /var/log/s3/copy_delete_queue.log 2>&1
{% endif %}
