{% from slspath + "/map.jinja" import conductor_agent with context %}
include:
  - .services

{% if conductor_agent.clean_conductor_agent_configs is not defined %}
  {% do conductor_agent.update({"clean_conductor_agent_configs": False}) %}
{% endif %}

{% if conductor_agent.enable_force_yes is not defined %}
  {% do conductor_agent.update({"enable_force_yes": False}) %}
{% endif %}

{% if conductor_agent.enable_safe_downgrade is not defined %}
  {% do conductor_agent.update({"enable_safe_downgrade": False}) %}
{% endif %}

{% if conductor_agent.overwrite is not defined %}
  {% do conductor_agent.update({"overwrite": False}) %}
{% endif %}

{%- if conductor_agent.clean_conductor_agent_configs %}
{%- for cfg in conductor_agent.not_managed_cfgs %}
{{ cfg }}:
  file.absent:
    - watch_in:
      - service: {{ conductor_agent.service }}
{%- endfor %}
{%- endif %}

common_configs:
  file.managed:
    - name: /etc/conductor-agent/conf.d/80-common.conf
    - source: salt://templates/conductor-agent/files/common.conf
    - template: jinja
    - makedirs: True
    - context:
        force_yes: {{ conductor_agent.get("enable_force_yes") }}
        safe_downgrade: {{ conductor_agent.get("enable_safe_downgrade") }}
        slsname: {{ slspath }}
        overwrite: {{ conductor_agent.get("overwrite") }}
    - watch_in:
      - service: {{ conductor_agent.service }}

root_conductor_config:
  file.managed:
    - name: /etc/conductor-agent/conductor-agent.conf
    - source: salt://templates/conductor-agent/files/conductor-agent.conf
    - template: jinja
    - makedirs: True
    - watch_in:
      - service: {{ conductor_agent.service }}

{% if conductor_agent.rules is defined %}
{% for g in conductor_agent.rules %}
{{ g.name }}.conf:
  file.managed:
    - name: /etc/conductor-agent/conf.d/{{ g.name }}.conf
    - source: salt://templates/conductor-agent/files/schedule.conf
    - template: jinja
    - context:
        group: {{ g.name }}
        pkglist: {{ g.get("packages", []) }}
        sched: {{ g.get("schedule", {}) }}
        slsname: {{ slspath }}
    - makedirs: True
    - watch_in:
      - service: {{ conductor_agent.service }}
{% endfor %}
{% endif %}
