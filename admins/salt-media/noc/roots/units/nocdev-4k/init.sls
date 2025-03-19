/etc/checkist/config.json:
  file.managed:
    - source: salt://{{slspath}}/files/etc/checkist/config.json
    - template: jinja
    - user: root
    - group: root
    - mode: 644
    - makedirs: True

/etc/checkist/id_rsa:
  file.managed:
    - source: salt://{{slspath}}/files/etc/checkist/id_rsa
    - template: jinja
    - user: root
    - group: root
    - mode: 600
    - makedirs: True

/etc/nginx/sites-enabled:
  file.recurse:
    - source: salt://{{slspath}}/files/etc/nginx/sites-enabled/
    - template: jinja

/etc/default:
  file.recurse:
    - source: salt://{{slspath}}/files/etc/default/

rsyslog-configs:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - names:
      - /etc/rsyslog.d/20-checkist-to-unified-agent.conf:
        - source: salt://{{slspath}}/files/etc/rsyslog.d/20-checkist-to-unified-agent.conf
      - /etc/rsyslog.d/20-nginx-to-unified-agent.conf:
        - source: salt://{{slspath}}/files/etc/rsyslog.d/20-nginx-to-unified-agent.conf
      - /etc/rsyslog.d/30-checkist.conf:
        - source: salt://{{slspath}}/files/etc/rsyslog.d/30-checkist.conf


/etc/logrotate.d/checkist:
  file.managed:
    - source: salt://{{slspath}}/files/logrotate.d/checkist
    - user: root
    - group: root
    - mode: 644
/etc/cron.d/checkist:
  file.managed:
    - source: salt://{{slspath}}/files/etc/cron.d/checkist
    - user: root
    - group: root
    - mode: 644
