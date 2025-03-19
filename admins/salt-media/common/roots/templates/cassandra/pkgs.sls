{% set is_stock = salt["pillar.get"]("cassandra:stock") %}
{% set version = salt["pillar.get"]("cassandra:version", 3.11.0) %}

cassandra_packages:
  pkg.installed:
    - pkgs:
    {%- for package in salt['pillar.get']('cassandra:pkgs') %}
      {%- if is_stock and package == "cassandra" %}
      - cassandra: {{version}}
      {%- else %}
      - {{ package }}
      {%- endif %}
    {%- endfor %}
