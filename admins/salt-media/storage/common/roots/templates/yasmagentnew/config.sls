# pillars:
#  yasmagent-defaults: default tags for instances
#   Example:
#     yasmagent-defaults:
#        host: {{ grains['conductor']['fqdn'] }}
#        geo: {{ grains['conductor']['root_datacenter'] }}
#        ctype: {{ grains['yandex-environment'] }}
#  yasmagent-instances: itype and tags
#   Example:
#     yasmagent-instances:
#       mdsstorage:
#         prj: someshit
#       porto:
#       ape:


{% set defaults = pillar.get('yasmagent-defaults', {}) %}
{% set instances = pillar.get('yasmagent-instances', []) %}
{% set rawparams = pillar.get('yasmagent-rawparams', []) %}
{% set rawstrings = pillar_config.get('yasmagent-rawstrings', []) %}

# --instance-getter echo s83vla.storage.yandex.net:100500@mdsspacemimic a_prj_none a_ctype_production a_geo_vla a_tier_none a_itype_mdsspacemimic
{% macro instance_getter(host, itype, prj='none', ctype='none', geo='none', tier='none') -%}
  {{ "--instanse-getter 'echo " ~ host ":100500@" ~ itype " a_prj_" ~ prj " a_ctype" ~ ctype " a_geo_" ~ geo " a_tier_" ~ tier " a_itype_" ~ itype ~ "'" }}
{%- endmacro %}

{%- macro raw_param(name, value) -%}
  {%- if value is string -%}
    {{ " --%s '%s'"|format(name, value) }}
  {%- elif value is sequence -%}
    {%- for item in value -%}
      {{ " --%s '%s'"| format(name, item) }}
    {%- endfor -%}
  {%- elif value is none or value == true -%}
    {{ " --%s"|format(name) }}
{%- endmacro -%}

{% set instances_line = instances|%}

/etc/default/yasmagent:
  file.managed:
    - source: salt://templates/yasmagentnew/files/yasmagent
    - mode: 644 
    - user: root
    - group: root
    - template: jinja
    - contents: |
      instance="{{ }}"
