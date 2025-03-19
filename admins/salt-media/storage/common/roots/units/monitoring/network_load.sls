{% set config = pillar.get('monitoring', {}).get('network_load', {}) %}
{% set crit_limit = config.get('crit_limit', 85) %}
{% set warn_limit = config.get('warn_limit', 75) %}
{% set time = config.get('time', 8) %}
{% set network_stats = config.get('network_stats', 'all') %}
{% set interface = config.get('interface', 'default') %}

/etc/monitoring/network_load.conf:
  file.managed:
    - name: /etc/monitoring/network_load.conf
    - contents: |
        CRIT_LIMIT={{ crit_limit }}
        WARN_LIMIT={{ warn_limit }}
        TIME={{ time }}
        NETWORK_STATS={{ network_stats }}
        INTERFACE={{ interface }}
    - user: root
    - group: root
    - mode: 644
  monrun.present:
    - name: network_load
    - command: "/usr/bin/network_load.sh"
    {% if (time * 3) | int >= 300   -%}
    - execution_interval: {{ (time * 3) | int }}
    {%- else -%}
    - execution_interval: 300
    {%- endif %}
    - execution_timeout:  {{ (time * 2) | int }}
