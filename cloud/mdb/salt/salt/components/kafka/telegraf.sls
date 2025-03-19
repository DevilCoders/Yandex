extend:
  /etc/telegraf/telegraf.conf:
    file:
    - source: salt://{{ slspath }}/conf/telegraf.conf

/etc/sudoers.d/monitoring:
  file.managed:
  - source: salt://{{ slspath }}/conf/telegraf.sudoers
  - mode: 440
  - template: jinja

/var/log/dbaas-billing:
  file.directory:
    - user: telegraf
    - group: telegraf
    - mode: 0755
    - require:
      - user: telegraf

/var/log/dbaas-billing/billing.log:
  file.managed:
    - user: telegraf
    - group: telegraf
    - mode: 0644
    - replace: False
    - require:
      - file: /var/log/dbaas-billing
