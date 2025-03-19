{% set ignored_interfaces = pillar.get('iface_ip_ignored_interfaces', 'lo|docker|dummy|vlan688') %}
/etc/monitoring/iface_ip.conf:
  file.managed:
    - name: /etc/monitoring/iface_ip.conf
    - contents: |
        ignored_interfaces="{{ ignored_interfaces }}"
    - user: root
    - group: root
    - mode: 644
