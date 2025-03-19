auditd:
  pkg.purged

systemd-journald-audit.socket:
  service.masked

systemd-journald.service:
  service.running:
    - watch:
       - systemd-journald-audit.socket

{%- if 'sec_packages' in pillar['common'] and 'osquery' in pillar['common']['sec_packages'] %}
osquery:
  pkg.purged

osquery-vanilla:
  pkg.installed:
    - name: osquery-vanilla
    - version: {{pillar['common']['sec_packages']['osquery']}}
{%- endif %}

{%- if 'sec_packages' in pillar['common'] and 'osquery_cfg' in pillar['common']['sec_packages'] %}
osquery-ycloud-svc-mrkt-config:
  pkg.purged

/etc/osquery.tag:
  file.managed:
    - source: salt://security/files/osquery.tag
    - user: root
    - group: root
    - mode: 0644


osquery-yandex-generic-config:
  pkg.installed:
    - name: osquery-yandex-generic-config
    - version: {{pillar['common']['sec_packages']['osquery_cfg']}}
{%- endif %}
