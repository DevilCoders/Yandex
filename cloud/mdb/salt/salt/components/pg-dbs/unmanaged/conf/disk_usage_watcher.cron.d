MAILTO=''
{% from "components/postgres/pg.jinja" import pg with context %}
*/1 * * * * postgres sleep $((RANDOM \% 20)); flock -n /tmp/disk_usage_watcher.lock /usr/bin/timeout 30 /usr/local/yandex/disk_usage_watcher.py --soft {{ salt['pillar.get']('data:pg:disk_usage_watcher_limit', 97) }} -p /var/lib/postgresql 2>&1 | logger -t 'disk_usage_watcher'
