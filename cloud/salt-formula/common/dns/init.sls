{% set hostname = grains['nodename'] %}

include:
  - common.pkgs

resolvconf:
  debconf.set:
    - data:
        'resolvconf/reboot-recommended-after-removal': {'type': 'note', 'value': 'ok' }
    - require:
      - yc_pkg: common_pkgs
  pkg.removed:
    - require:
      - debconf: resolvconf

bind9:
  pkg.removed

config-caching-dns:
  yc_pkg.installed:
    - pkgs:
      - config-caching-dns

/etc/default/config-caching-dns:
  file.managed:
    - source: salt://{{ slspath }}/files/config-caching-dns
    - require:
      - yc_pkg: config-caching-dns

config-caching-dns-update-configs:
  cmd.run:
    - name: config-caching-dns-update-configs
    - onchanges:
      - file: /etc/default/config-caching-dns
    - require:
      - yc_pkg: config-caching-dns
      - file: /etc/default/config-caching-dns

{{ sls }}-default_hostfile:
  host.present:
    - ip: "{{ grains['cluster_map']['hosts'][hostname]['ipv6']['addr'] }}"
    - names:
      - "{{ grains['nodename'] }}"
      - "{{ grains['host'] }}"

{{ sls }}-ipv6_localhost:
  host.present:
    - ip: '::1'
    - names:
      - 'localhost'
      - 'ip6-localhost'
      - 'ip6-loopback'
