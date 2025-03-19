/etc/noc-ck/config.yml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/noc-ck/config.yml-{{grains["yandex-environment"]}}
    - template: jinja
    - user: root
    - group: root
    - mode: 644
    - makedirs: True

/etc/noc-ck/env:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/noc-ck/env
    - template: jinja
    - user: root
    - group: root
    - mode: 644
    - makedirs: True

robot-noc-ck:
  user.present:
    - createhome: True
    - system: True
    - home: /home/robot-noc-ck
  pkg.installed:
    - pkgs:
      - python3.7
    - skip_suggestions: True
    - install_recommends: True

/home/robot-noc-ck/.ssh/id_rsa:
  file.managed:
    - source: salt://{{ slspath }}/files/home/robot-noc-ck/.ssh/id_rsa
    - template: jinja
    - user: robot-noc-ck
    - group: root
    - mode: 600
    - makedirs: True

/home/robot-noc-ck/.rtapi/token.txt:
  file.managed:
    - source: salt://{{ slspath }}/files/home/robot-noc-ck/.rtapi/token.txt
    - template: jinja
    - user: robot-noc-ck
    - group: root
    - mode: 600
    - makedirs: True

/etc/nginx/sites-enabled:
  file.recurse:
    - source: salt://{{ slspath }}/files/etc/nginx/sites-enabled/
    - template: jinja

/etc/logrotate.d/noc-ck:
  file.managed:
    - source: salt://{{slspath}}/files/etc/logrotate.d/noc-ck
    - template: jinja
    - user: root
    - group: root
    - mode: 600

rsyslog-configs:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - names:
      - /etc/rsyslog.d/20-noc-ck-to-unified-agent.conf:
        - source: salt://{{slspath}}/files/etc/rsyslog.d/20-noc-ck-to-unified-agent.conf
      - /etc/rsyslog.d/20-nginx-to-unified-agent.conf:
        - source: salt://{{slspath}}/files/etc/rsyslog.d/20-nginx-to-unified-agent.conf
      - /etc/rsyslog.d/30-noc-ck.conf:
        - source: salt://{{slspath}}/files/etc/rsyslog.d/30-noc-ck.conf
