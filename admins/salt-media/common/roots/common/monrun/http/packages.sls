{% from slspath + "/map.jinja" import py_version with context %}

pkgs:
  pkg.installed:
    - pkgs:
{% if py_version == "3" %}
      - python3-yaml
      - python3-requests
{% else %}
      - python-yaml
      - python-requests
{% endif %}
