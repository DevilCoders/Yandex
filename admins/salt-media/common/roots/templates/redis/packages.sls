{% from slspath + "/map.jinja" import common with context %}

redis_packages:
  pkg.installed:
    - pkgs:
      {%- for pkg in common.packages %}
      - {{ pkg }}
      {%- endfor %}
