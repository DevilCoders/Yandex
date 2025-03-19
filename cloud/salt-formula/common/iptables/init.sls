# REMOVEME
/usr/local/sbin/iptables-save: file.absent

# REMOVEME
/usr/local/sbin/ip6tables-save:
  file.absent:
    - require:
      - file: /usr/local/sbin/iptables-save


{% set host_base_role = grains["cluster_map"]["hosts"][grains["nodename"]]["base_role"] -%}
{% if grains["cluster_map"]["environment"] not in ("dev", "testing", "hw-ci") and host_base_role != "cloudvm" -%}

{{ sls }}_pkgs:
  yc_pkg.installed:
    - pkgs:
      - iptables
      - iptables-persistent
      - netfilter-persistent

/opt/yc-iptables/sbin:
  file.directory:
    - user: root
    - group: root
    - mode: 750
    - makedirs: True

/opt/yc-iptables/sbin/iptables-save:
  file.managed:
    - source: salt://{{ slspath }}/files/iptables-save
    - mode: 740
    - require:
      - file: /opt/yc-iptables/sbin
      - yc_pkg: {{ sls }}_pkgs

/etc/iptables/rules.v4:
  file.managed:
    - source: salt://{{ slspath }}/files/rules.v4
    - template: jinja
    - mode: 640
    - require:
      - yc_pkg: {{ sls }}_pkgs

/etc/iptables/rules.v6:
  file.managed:
    - source: salt://{{ slspath }}/files/rules.v6
    - mode: 640
    - template: jinja
    - require:
      - yc_pkg: {{ sls }}_pkgs

/opt/yc-iptables/sbin/ip6tables-save:
  file.symlink:
    - target: /opt/yc-iptables/sbin/iptables-save
    - force: True
    - require:
      - file: /opt/yc-iptables/sbin/iptables-save

/etc/default/netfilter-persistent:
  file.managed:
    - source: salt://{{ slspath }}/files/netfilter-persistent
    - mode: 640
    - require:
      - yc_pkg: {{ sls }}_pkgs

rules_loaded:
  cmd.script:
    - source: salt://{{ slspath }}/misc/check_rules
    - stateful: True
    - args: 'salt_test'
    - require:
      - file: /opt/yc-iptables/sbin/ip6tables-save
      - file: /opt/yc-iptables/sbin/iptables-save
      - file: /etc/iptables/rules.v4
      - file: /etc/iptables/rules.v6
      # REMOVEME
      - file: /usr/local/sbin/ip6tables-save

netfilter-persistent:
  service.running:
    - enable: True
    - restart: True
    - require:
      - file: /etc/default/netfilter-persistent
      - file: /etc/iptables/rules.v4
      - file: /etc/iptables/rules.v6
      - file: /opt/yc-iptables/sbin/ip6tables-save
      - file: /opt/yc-iptables/sbin/iptables-save
    - watch:
      # On service restart all the HBF-installed rules && chains will be dropped. Hoping it will restore 'em later.
      - file: /etc/iptables/rules.v4
      - file: /etc/iptables/rules.v6
      - cmd: rules_loaded

rules_applied:
  cmd.script:
    - source: salt://{{ slspath }}/misc/check_rules
    - require:
      # REMOVEME
      - file: /usr/local/sbin/ip6tables-save
    - watch:
      - netfilter-persistent
{% endif -%}
