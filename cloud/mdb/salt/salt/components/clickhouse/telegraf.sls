extend:
  /etc/telegraf/telegraf.conf:
    file:
    - source: salt://{{ slspath }}/conf/telegraf.conf

/etc/sudoers.d/telegraf:
  file.managed:
  - source: salt://{{ slspath }}/conf/telegraf.sudoers
  - mode: 440
  - template: jinja

/etc/telegraf/limits.json:
  fs.file_present:
  - contents_function: mdb_clickhouse.render_limits_config
  - user: root
  - group: telegraf
  - mode: 640
  - makedirs: True

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
