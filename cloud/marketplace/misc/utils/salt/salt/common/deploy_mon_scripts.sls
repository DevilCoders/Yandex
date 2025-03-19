{% set config = '/home/monitor/agents/etc' %}
{% set script = '/home/monitor/agents/modules' %}

{%- for scope in ('local', 'common') %}
  {%- if scope in monitoring %}
    {%- set path = 'common' if scope == 'common' else slspath -%}
    {%- for name,service in monitoring[scope].iteritems() %}
      {%- if 'conf' in service %}
{{ config }}/{{ service['conf'] }}:
  file.managed:
    - source: salt://{{ path }}/mon/etc/{{ service['conf'] }}
    - template: jinja
    - makedirs: True
      {%- endif %}
      {% if 'file' in service %}
        {%- set command = '/'.join((script, service['file'])) %}
{{ command }}:
  file.managed:
    - source: salt://{{ path }}/mon/bin/{{ service['file'] }}
    - template: jinja
    - makedirs: True
    - mode: 0755
{% set check_name = service['file'].split('.')[0] -%}
{%- include "common/monrun_check.sls" %}
      {%- endif %}
      {% if 'link' in service %}
        {%- set command = '/'.join((script, name)) %}
{{ command }}:
  file.symlink:
    - target: {{ service['link'] }}
    - makedirs: True
    - force: True
        {%- set check_name = name.split('.')[0] %}
        {%- include "common/monrun_check.sls" %}
      {%- endif %}
    {%- endfor %}
  {%- endif %}
{%- endfor %}