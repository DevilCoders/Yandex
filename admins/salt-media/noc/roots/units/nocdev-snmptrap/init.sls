snmptrapper:
  user.present:
    - name: snmptrapper
    - system: True
  pkg.latest:
    - refresh: true
  service.running:
    - enable: True
    - name: snmptrapper

/etc/snmptrapper/config.yml:
  file.managed:
  - source: salt://{{ slspath }}/files/etc/snmptrapper/config.yml
  - template: jinja
  - user: snmptrapper
  - group: snmptrapper
  - mode: 600
  - makedirs: True

/etc/yandex/unified_agent/conf.d:
  file.recurse:
    - source: salt://{{ slspath }}/files/etc/yandex/unified_agent/conf.d
    - template: jinja
    - user: unified_agent
    - group: unified_agent
    - file_mode: 644
    - dir_mode: 755
    - makedirs: True
    - clean: True

/etc/yandex/unified_agent/secrets/tvm:
  file.managed:
    - user: unified_agent
    - group: unified_agent
    - mode: 400
    - dir_mode: 750
    - makedirs: True
    - contents_pillar: 'unified_agent:tvm-client-secret'

telegraf:
  service.running:
    - reload: True
    - require:
        - pkg: telegraf
  pkg:
    - installed
    - pkgs:
        - telegraf-noc-conf
        - telegraf
