{% set cluster = pillar.get('cluster') %}

include:
    - templates.packages
    - templates.lxd
    - units.chrony
    - .watchdog

/etc/modules-load.d/03-tunnel.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 644
      contents: |
        ip6_tunnel
        ip_tunnel

/etc/modules-load.d/03-lxd-br-netfilter.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 644
      contents: |
        br_netfilter

/etc/modules-load.d/03-k8s.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 644
      contents: |
        overlay
        ip_vs
        ip_vs_sh
        ip_vs_rr
        ip_vs_wrr

# kube proxy try to change hashsize parameter of nf_conntrack if it not equil 2097152
# https://www.claudiokuenzler.com/blog/1106/unable-to-deploy-rancher-managed-kubernetes-cluster-lxc-lxd-nodes
/etc/modprobe.d/k8s-conntrack.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 644
      contents: |
        options nf_conntrack hashsize=2097152

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

# load nf_conntrack before systemd-sysctl. see NOCDEVDUTY-894
nf_conntrack:
  kmod.present:
    - mods:
      - nf_conntrack
    - persist: True

ipip:
  kmod.present:
    - mods:
      - ipip
    - persist: True

# load sysctl if nf_conntrack was loaded
reload sysctl:
  service.running:
    - name: systemd-sysctl
    - restart: True
    - watch:
      - kmod: nf_conntrack

Yandex HBF Agent:
  file.managed:
    - name: /etc/yandex-hbf-agent/yandex-hbf-agent.conf
    - makedirs: True
    - user: root
    - group: root
    - mode: 644
    - source: salt://files/dom0/yandex-hbf-agent.conf
  pkg.installed:
    - pkgs:
      - yandex-hbf-agent-static
      - yandex-hbf-agent-init
  group.present:
    - name: lxd
    - addusers:
      - hbf
  service.running:
    - name: yandex-hbf-agent
    - enable: True
    - watch:
      - group: lxd # match by 'name' https://docs.saltproject.io/en/latest/ref/states/requisites.html#identifier-matching
    - require:
      - group: lxd # first modify the group, then restart NOCDEV-6129

/usr/share/yandex-hbf-agent/rules.d/04-hbf-exceptions.v4:
  file.managed:
    - source: salt://{{ slspath }}/usr/share/yandex-hbf-agent/rules.d/04-hbf-exceptions.v4
/usr/share/yandex-hbf-agent/rules.d/04-hbf-exceptions.v6:
  file.managed:
    - source: salt://{{ slspath }}/usr/share/yandex-hbf-agent/rules.d/04-hbf-exceptions.v6
/usr/share/yandex-hbf-agent/rules.d/04-gray-networks.v4:
  file.managed:
    - source: salt://{{ slspath }}/usr/share/yandex-hbf-agent/rules.d/04-gray-networks.v4
/usr/share/yandex-hbf-agent/rules.d/04-k8s-exceptions.v6:
  file.managed:
    - source: salt://{{ slspath }}/usr/share/yandex-hbf-agent/rules.d/04-k8s-exceptions.v6

/usr/bin/hwinf:
  file.managed:
    - user: root
    - group: root
    - mode: 755
    - source: salt://{{ slspath }}/usr/bin/hwinf


{% if "mysql" in salt.cmd.run("lxc ls --format=csv -cn") %}
# Создаём каталог /mysqld только если его уже нет.
# После салтификации внутри контейнера с mysql-ем у этого каталога поменяются права.
# Права поменяются на mysql.mysql 744.
# Поэтому мы не трогаем его, если он уже есть, и поэтому тут не file.directory.
mkdir /mysqld; chmod 777 /mysqld:
  cmd.run:
    - unless: test -e /mysqld
{% endif %}
