{% set prefix = pillar['network']['tmp_config_prefix'] %}

yc-network-config-pkg:
  yc_pkg.installed:
    - pkgs:
      - yc-network-config
{%- if grains['virtual'] == 'physical' %}
    - disable_update: True
{% endif %}

network_config_init:
  file.directory:
    - names:
      - {{ prefix }}/network/if-down.d
      - {{ prefix }}/network/if-post-down.d
      - {{ prefix }}/network/if-pre-up.d
      - {{ prefix }}/network/if-up.d
      - {{ prefix }}/network/interfaces.d
    - makedirs: True

networking_stopped:
  service.dead:
    - name: networking

interfaces_d_clean:
  file.directory:
    - names:
      - /etc/network/interfaces.d
      - /etc/network/if-pre-up.d
      - /etc/network/if-down.d
      - /etc/network/if-post-down.d
      - /etc/network/if-up.d
    - clean: True
    - exclude_pat: '*_yc_*'
    - onchanges:
      - service: networking_stopped

networking_started:
  service.running:
    - name: networking
    - enable: True
