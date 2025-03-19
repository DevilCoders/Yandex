{% set prefix = pillar['network']['tmp_config_prefix'] %}
{% set underlay_interfaces = salt['underlay.interfaces']() %}

{%- if grains['is_mellanox'] %}

{{ prefix }}/network/interfaces.d/30_yc_cloudgate_cfg:
  file.managed:
    - name: {{ prefix }}/network/interfaces.d/30_yc_cloudgate_cfg
    - template: jinja
    - source: salt://{{ slspath }}/files/30_yc_cloudgate_cfg
    - defaults:
        data_dev: {{ underlay_interfaces.dataplane }}
        inet6_dev: {{ underlay_interfaces.upstream_ipv6 }}
        inet_dev: {{ underlay_interfaces.upstream_ipv4 }}
    - require:
      - service: configure-underlay
      - file: network_config_init

/etc/network/interfaces.d/30_yc_cloudgate_cfg:
  file.copy:
    - source: {{ prefix }}/network/interfaces.d/30_yc_cloudgate_cfg
    - preserve: True
    # Overwrite even if already present
    - force: True
    - require:
      - file: interfaces_d_clean

extend:
  networking_stopped:
    service:
      - onchanges:
        - file: {{ prefix }}/network/interfaces.d/30_yc_cloudgate_cfg
  networking_started:
    service:
      - require:
        - file: /etc/network/interfaces.d/30_yc_cloudgate_cfg

{%- endif %}
