
{%- set hostname = grains['nodename'] %}
{%- set host_tags = salt['grains.get']('cluster_map:hosts:%s:tags' % hostname) %}
{%- set host_roles = grains['cluster_map']['hosts'][hostname]['roles'] -%}


#TODO: remove then vpp 18.10 is adopted
{%- if 'cgw-nat' in host_roles or 'cgw-dc' in host_tags %}
remove-cgw-pkgs-old:
  pkg.removed:
    - pkgs:
      - vpp: 18.04-17.15.123
      - vpp-dbg: 18.04-17.15.123
      - vpp-lib: 18.04-17.15.123
      - vpp-api-python: 18.04-17.15.123
      - vpp-plugins: 18.04-17.15.123
    - require_in:
      yc_pkg: yandex-cloudgate-pkgs
{%- endif %}


yandex-cloudgate-pkgs:
  yc_pkg.installed:
    - pkgs:
      - vpp
      - vpp-dbg
      - vpp-lib
      - vpp-api-python
      - vpp-plugins

{%- if 'cgw-nat' in host_roles or 'cgw-dc' in host_tags %}
{#TODO: after CLOUD-15512 will be unified #}
      - vpp-ext-deps
{%- else %}
      - vpp-dpdk-dev
      - vpp-dpdk-dkms
{%- endif %}
      - vppsb-router-plugin
      - yandex-vpp-plugins
      - yandex-vpp-scripts
      - gobgp
{%- if 'bgp2vpp' in host_tags %}
      - yc-bgp2vpp
{%- else %}
       - yc-cgw-prosector
{%- endif %}
{#until CLOUD-7747 we need this for kernel 4.14 dkms dependencies resolution #}
      - libelf-dev
{%- if grains['is_mellanox'] %}
      - ibverbs-providers
      - mlnx-ofed-kernel-dkms #kernel drivers to configure mellanox ifaces. FIXME: replace with only kernels modules without dkms see CLOUD-5358
{%- endif %}
    - require_in:
      service: solomon-agent


vppctl-mode:
  file.managed:
    - name: /usr/bin/vppctl
    - group: vpp
    - mode: '2755'  # setgid -rwxr-sr-x
    - replace: False
    - require:
      - yc_pkg: yandex-cloudgate-pkgs
    - require_in:
        service: vpp-service
