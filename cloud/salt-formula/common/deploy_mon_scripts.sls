{#
  There can be cmd statement in monitoring description.
  If there is, cmd must be used as command string on monitoring execution
  If there isn't, command string must be composed from file or link statement.
  See CLOUD-19930
#}

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
        {%- if service.get('monrun_check', True) %}
{% set check_name = service['file'].split('.')[0] -%}
{%- include "common/monrun_check.sls" %}
        {%- endif %}
        {%- if service.get('juggler_check', False) %}
{%- include "common/juggler_check.sls" %}
        {%- endif %}
      {%- endif %}
      {% if 'link' in service %}
        {%- set command = '/'.join((script, name)) %}
{{ command }}:
  file.symlink:
    - target: {{ service['link'] }}
    - makedirs: True
    - force: True
{% set check_name = name.split('.')[0] %}
{%- include "common/monrun_check.sls" %}
      {%- endif %}
      {%- if 'cmd' in service %}
{% set check_name = name.split('.')[0] %}
{%- set command = '/'.join((script, service['cmd'])) %}
{%- include "common/monrun_check.sls" %}
      {%- endif %}
    {%- endfor %}
  {%- endif %}
{%- endfor %}
