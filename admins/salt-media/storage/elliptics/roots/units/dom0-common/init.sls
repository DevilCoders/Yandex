{% set cluster = pillar.get('cluster') %}

include:
    - templates.packages
    - templates.lxd
    - templates.selfdns
    - templates.nginx
    - units.iface-ip-conf
    - .nginx
    - .watchdog

/etc/modules-load.d/03-tunnel.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 644
      contents: |
        ip6_tunnel
        ip_tunnel

/etc/systemd/system/lxd.service.d/ulimit_memlock.conf:
  file.managed:
    - makedirs: True
    - user: root
    - group: root
    - mode: 644
    - source: salt://files/dom0/lxd_ulimit_memlock.conf

/etc/rc.local:
  file.managed:
    - source: salt://files/dom0/rc.local
    - mode: 755

systemctl daemon-reload:
  cmd.run:
    - onchanges:
      - file: /etc/systemd/system/lxd.service.d/ulimit_memlock.conf

net.ipv6.conf.all.use_tempaddr:
  sysctl.present:
    - value: 0

net.ipv6.conf.default.use_tempaddr:
  sysctl.present:
    - value: 0

/etc/yandex-hbf-agent/yandex-hbf-agent.conf:
  file.managed:
    - makedirs: True
    - user: root
    - group: root
    - mode: 644
    - source: salt://files/dom0/yandex-hbf-agent.conf

statbox-push-client:
  service.disabled

apparmor:
  service.disabled

ureadahead:
  service.disabled
