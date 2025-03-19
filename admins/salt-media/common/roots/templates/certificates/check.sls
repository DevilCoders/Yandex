{%- from slspath + "/map.jinja" import certificates with context -%}
{% if salt['pillar.get']('ignore_conductor', false) %}
{%- set check, version = (certificates.check_pkg, None) -%}
{% else %}
{%- set check = salt['conductor.package'](certificates.check_pkg) -%}
{%- if check -%}
{%- set check, version = check[0].items()[0] -%}
{%- else -%}
{%- set check, version = (certificates.check_pkg, None) -%}
{%- endif -%}
{% endif %}


{{ check }}:
{%- if version %}
  pkg.installed:
    - version: "{{ version }}"
{%- else %}
  pkg.installed
{%- endif %}
