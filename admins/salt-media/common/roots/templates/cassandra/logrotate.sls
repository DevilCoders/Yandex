{% from slspath + "/map.jinja" import cassandra with context %}
/etc/logrotate.d/cassandra-gc:
{% if cassandra.logrotate_enabled %}
  file.managed:
    - mode: 644
    - contents: |
        /var/log/cassandra/gc.log {
            rotate 14
            daily
            missingok
            compress
            delaycompress
            copytruncate
            daily
            notifempty
        }
{% else %}
  file.absent
{% endif %}
