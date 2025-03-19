# Copied from https://bb.yandex-team.ru/projects/CLOUD/repos/salt-formula/browse/slb/init.sls
ipvs_tun-sysctl:
  file.managed:
    - name: /etc/sysctl.d/99-ipvs_tun.conf
    - mode: '0664'
    - source: salt://files/slb/99-ipvs_tun.conf

/etc/modules-load.d/tunnel.conf:
  file.managed:
    - contents:
      - 'dummy'
      - 'ipip'
      - 'ip6_tunnel'
      - 'tunnel6'

python-netconfig:
  pkg.installed:
    - pkgs:
      - python-netconfig

/etc/netconfig.d/01-disable-eth0-config.py:
  file.managed:
    - name: 
    - source: salt://files/slb/netconfig_plugin.py
    - template: jinja
    - defaults:
        iface: eth0

/usr/bin/netconfig-slb.sh:
  file.managed:
    - source: salt://files/slb/netconfig-slb.sh
    - mode: '0755'

/lib/systemd/system/netconfig-slb.service:
  file.managed:
    - source: salt://services/slb/netconfig-slb.service

netconfig_slb_service:
  service.enabled:
    - name: netconfig-slb.service