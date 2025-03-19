lbs_reinit:
  cmd.run:
    - name: '/usr/sbin/yc-lbs-reinit-hdds --yes-please-destroy-all-data-in-lbs >> /var/log/test_lbs_reinit_logs 2>&1 && touch /etc/yc/lbs/reinit.done'
    - unless: 'test -f /etc/yc/lbs/reinit.done'
    - require:
      - yc_pkg: yc-lbs
      - file: /etc/yc/lbs/config.toml

lbs_test_layout:
  file.managed:
    - name: /usr/bin/yc-lbs-init
    - source: salt://{{ slspath }}/yc-lbs-init
    - template: jinja
    - user: root
    - group: root
    - mode: 755
  cmd.run:
    - name: '/usr/bin/yc-lbs-init && touch /etc/yc/lbs/initlbs.done'
    - unless: 'test -f /etc/yc/lbs/initlbs.done'
    - require:
      - service: yc-lbs
      - file: /usr/bin/yc-lbs-init
      - cmd: lbs_reinit
