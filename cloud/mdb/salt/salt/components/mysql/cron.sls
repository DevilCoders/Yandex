{% set cron_scripts = [
    'mysql_rotate_binlogs',
    'mysql_autoflush_hosts',
    'mysql_purge_binlogs',
    'mysql_resetup',
    'mysql_mdb_repl_mon',
    'mysql_innodb_status_output',
    'mysql_flush_logs',
    'mysql_rotate_perf_schema'
] %}
{% for cs in cron_scripts %}
/etc/cron.d/{{ cs }}:
  file.managed:
    - source: salt://{{ slspath }}/conf/cron.d/{{ cs }}
    - template: jinja
    - mode: 644
{% endfor %}
