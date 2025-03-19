{% set prefix = pillar['network']['tmp_config_prefix'] %}
{% set underlay_interfaces = salt['underlay.interfaces']() %}

include:
  - common.network_interfaces

{% if 'dataplane' in underlay_interfaces %}
{{ prefix }}/network/interfaces.d/10_yc_base_ipv4_cfg:
  file.managed:
    - name: {{ prefix }}/network/interfaces.d/10_yc_base_ipv4_cfg
    - template: jinja
    - source: salt://{{ slspath }}/files/10_yc_base_ipv4_cfg
    - defaults:
        inet_dev: {{ underlay_interfaces.dataplane }}
    - require:
      - file: network_config_init

/etc/network/interfaces.d/10_yc_base_ipv4_cfg:
  file.copy:
    - source: {{ prefix }}/network/interfaces.d/10_yc_base_ipv4_cfg
    - preserve: True
    # Overwrite even if already present
    - force: True
    - require:
      - file: interfaces_d_clean
{% endif %}

extend:
  networking_stopped:
    service:
      - onchanges:
        {% if 'dataplane' in underlay_interfaces %}
        - file: {{ prefix }}/network/interfaces.d/10_yc_base_ipv4_cfg
        {% endif %}
  networking_started:
    service:
      - require:
        {% if 'dataplane' in underlay_interfaces %}
        - file: /etc/network/interfaces.d/10_yc_base_ipv4_cfg
        {% endif %}
