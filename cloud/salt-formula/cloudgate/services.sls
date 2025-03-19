{# See CLOUD-14292.#}
{# While vpp, gobgp and go2vpp are binded we should restart them just once #}
{%- set hostname = grains['nodename'] %}
{%- set host_tags = salt['grains.get']('cluster_map:hosts:%s:tags' % hostname) %}

vpp-service:
  service.running:
    - name: vpp
    - enable: True
    - full_restart: True
    - require:
      - yc_pkg: yandex-cloudgate-pkgs
      - service: yavpp-config-ensure-on-start-service
      - service: openibd-service
{# VPP deps #}
      - kmod: vpp-dpdk-kernel-modules
      - file: vpp-conf
      - file: 10-yavpp-environment-conf-file
      - file: 20-yavpp-hugepages-conf-file
      - file: 30-yavpp-bindings-conf-file
      - file: hostifaces-conf
{# GoBGP deps #}
      - file: gobgp-conf
      - file: gobgp-service-file
{%- if 'bgp2vpp' in host_tags %}
{# bgp2vpp deps #}
      - file: bgp2vpp-conf
      - file: bgp2vpp-service-file
      - file: remove-go2vpp-files
      - service: govpp-stop
{% else %}
{# Go2VPP deps #}
      - file: go2vpp-conf
      - file: go2vpp-service-file
{% endif %}
    - watch:
    {# CLOUD-15328: manage service states using salt. #}
    {# TODO: split this watch into related to each service after services become unbound again #}
      - yc_pkg: yandex-cloudgate-pkgs
{# VPP deps #}
      - file: vpp-conf
      - file: 30-yavpp-bindings-conf-file
      - file: hostifaces-conf
      - file: vppctl-mode
{# GoBGP deps #}
      - file: gobgp-conf
      - file: gobgp-service-file
{%- if 'bgp2vpp' in host_tags %}
{# bgp2vpp deps #}
      - file: bgp2vpp-conf
      - file: bgp2vpp-service-file
{% else %}
{# Go2VPP deps #}
      - file: go2vpp-conf
      - file: go2vpp-service-file
{% endif %}

yavpp-config-ensure-on-start-service:
  service.running:
    - enable: True
    - name: yavpp-config-ensure-on-start
    - watch:
      - file: yavpp-config-ensure-on-start-service-file

yavpp-autorecovery-timer:
  service.running:
    - enable: True
    - name: yavpp-autorecovery.timer
    - require:
      - file: yavpp-autorecovery-timer-file
      - file: yavpp-autorecovery-service-file
    - watch:
      - file: yavpp-autorecovery-timer-file

{%- set modules = grains['yavpp_modules'] %}
vpp-dpdk-kernel-modules:
    kmod.present:
      - mods: {{ modules }}
      - persist: True
      - require:
        - yc_pkg: yandex-cloudgate-pkgs

{%- if 'bgp2vpp' in host_tags %}
remove-go2vpp-files:
  file.absent:
    - names:
      - /etc/vpp/go2vpp.conf
      - /etc/systemd/system/go2vpp.service

govpp-stop:
  service.dead:
      - name: go2vpp
      - enable: false
{% endif %}

openibd-service:
  service.running:
    - name: openibd
    - enable: True
    - full_restart: True
    - require:
      - yc_pkg: yandex-cloudgate-pkgs
