{% from slspath + "/map.jinja" import salt_minion with context %}

salt_minion:
  pkg.installed:
  - pkgs:
{% for package in salt_minion.packages %}
    - {{ package }}
{% endfor %}
  service.running:
    - name: {{ salt_minion.service }}
    - enable: True
