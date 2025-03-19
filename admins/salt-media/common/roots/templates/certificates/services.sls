{% from slspath + "/map.jinja" import certificates with context %}
{% if salt['pillar.get']('ignore_conductor', false) %}
{% set packages = False %}
{% else %}
{%- if certificates.packages  -%}
{%- set packages = salt['conductor.package'](certificates.packages) -%}
{%- else -%}
{% set packages = False %}
{%- endif -%}
{% endif %}

{%- set exclude_list = [
] -%}

{%- set s3_glob = {'skip_package': False} -%}
{%- for host in exclude_list -%}
  {%- if salt["grains.get"]("conductor:fqdn") == host -%}
    {%- set _ = s3_glob.update({'skip_package':True}) -%}
  {%- endif -%}
{%- endfor -%}

{%- if packages and not s3_glob['skip_package'] -%}
certificate_packages:
  pkg.installed:
    - pkgs:
    {%- for pkg in packages %}
      - {{ pkg }}
    {%- endfor %}
    {% if "repos" in pillar %}
    - require:
      - sls: templates.repos
    {% endif %}
  service.running:
    - name: {{ certificates.service }}
    - enable: True
    - reload: True
    - watch:
      {%- if certificates.get('files', []) %}
      - yafile: {{ certificates.path }}/*
      {%- else %}
      - file: {{ certificates.path }}/*
      {%- endif %}
{%- endif %}
