{% set checks = {
  'common': [
    'if_errors.sh',
    'ntp_stratum.sh',
    'load_average.sh',
    'ext_ip_dns.py',
    'yasmagent.py',
    'sumcores.sh',
    'salt_minion_version.py',
    'ro_fs.py',
    'apt_check_updates.py',
    'correct_dc.py',
    ],
  'pg-common': [
    'pg_walg_backup_age.py'
  ]}
%}

telegraf-scripts-deps:
  pkg.installed:
    - pkgs:
      - python3-dnspython

extend:
  /etc/telegraf/telegraf.conf:
    file:
      - source: salt://{{ slspath }}/conf/telegraf.conf

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

{% if salt['pillar.get']('data:billing:use_cloud_logbroker', False) %}
/var/log/dbaas-billing/billing-yc.log:
  file.managed:
    - user: telegraf
    - group: telegraf
    - mode: 0644
    - replace: False
    - require:
      - file: /var/log/dbaas-billing
{% endif %}

/etc/telegraf/scripts:
  file.directory:
    - user: telegraf
    - group: telegraf
    - mode: 0755

/etc/telegraf/scripts/mdstat-health.sh:
  file.managed:
    - source: salt://{{ slspath }}/conf/mdstat-health.sh
    - user: telegraf
    - group: telegraf
    - mode: 0755
    - template: jinja

/etc/telegraf/scripts/pxf-health.sh:
  file.managed:
    - source: salt://{{ slspath }}/conf/pxf-health.sh
    - user: telegraf
    - group: telegraf
    - mode: 0755
    - template: jinja

/usr/local/yandex/telegraf/scripts:
  file.directory:
    - makedirs: True

/usr/local/yandex/telegraf/scripts/errata_updates.py:
  file.managed:
    - source: salt://{{ slspath }}/conf/errata_updates.py
    - user: telegraf
    - group: telegraf
    - mode: 0755
    - template: jinja

/usr/local/yandex/telegraf/scripts/gp_xlog_files.py:
  file.managed:
    - source: salt://{{ slspath }}/conf/gp_xlog_files.py
    - user: telegraf
    - group: telegraf
    - mode: 0755
    - template: jinja

{% for type, checks in checks.items() %}
{% for check in checks %}
/usr/local/yandex/telegraf/scripts/{{ check }}:
  file.managed:
    - source: salt://components/monrun2/{{ type }}/scripts/{{ check }}
    - mode: 0755
    - require:
      - /usr/local/yandex/telegraf/scripts
{% endfor %}
{% endfor %}

/etc/sudoers.d/telegraf_recovery:
    file.managed:
        - source: salt://{{ slspath }}/conf/telegraf.sudoers
        - mode: 440
        - template: jinja

