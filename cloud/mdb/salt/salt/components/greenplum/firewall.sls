/etc/ferm/conf.d/10_greenplum.conf:
  file.managed:
    - source: salt://{{ slspath }}/conf/greenplum.iptables
    - template: jinja
    - mode: 644
    - require:
      - test: ferm-ready
    - watch_in:
      - cmd: reload-ferm-rules

include:
  - components.firewall.external_access_config
