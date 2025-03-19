net.ipv6.conf.all.forwarding:
  sysctl.present:
    - value: 1

net.ipv6.conf.eth0.accept_ra:
  sysctl.present:
    - value: 2

iptables-persistent:
  pkg.installed

netfilter-persistent:
  service.running:
    - enable: True
    - reload: True
    - require:
      - pkg: iptables-persistent
    - watch:
      - file: /etc/iptables/rules.v6

/etc/iptables/rules.v6:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/iptables/rules.v6
    - user: root
    - group: root
    - makedirs: True
    - mode: 0640
