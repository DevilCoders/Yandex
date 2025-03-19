ip-spoof:
  pkg.installed:
    - pkgs:
      - ipt-spoof-modules-5.4.161-26.3
      - ipt-ipspoof

  file.managed:
    - name: /etc/modules-load.d/xt_IPADDR.conf
    - contents: xt_IPADDR
    - mode: 644
    - user: root
    - group: root

ip-spoof-iptables:
  file.managed:
    - name: /etc/yandex-hbf-agent/rules.d/99-tun-src-randomize.v6
    - source: salt://units/ipip-randomize/files/99-tun-src-randomize.v6
    - user: root
    - group: root
    - mode: 644
    - template: jinja
    - context:
        iface: {{salt['cmd.shell']('source /usr/local/sbin/autodetect_active_eth; echo $default_route_iface')}}
