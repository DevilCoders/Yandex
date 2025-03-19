{%- from slspath + "/map.jinja" import yarl_vars with context -%}

include:
  - .ubic

/etc/yarl/allCAs.pem:
  file.managed:
    - contents_pillar: yarl-secrets:sslrootcert
    - mode: 644
    - user: root
    - group: root
    - makedirs: true

/etc/cron.d/yarl:
  file.absent

/etc/logrotate.d/yarl:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/logrotate.d/yarl

/etc/monrun/conf.d/yarl.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/monrun/conf.d/yarl.conf
    - template: jinja
    - makedirs: True

/usr/local/bin/yarl-check.py:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/local/bin/yarl-check.py
    - template: jinja
    - makedirs: True

yarl-errors:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/local/bin/yarl-errors.sh
    - name: /usr/local/bin/yarl-errors.sh
    - mode: 755
    - makedirs: True

  monrun.present:
    - command: /usr/local/bin/yarl-errors.sh
    - execution_interval: 300
    - execution_timeout: 30
    - type: yarl
