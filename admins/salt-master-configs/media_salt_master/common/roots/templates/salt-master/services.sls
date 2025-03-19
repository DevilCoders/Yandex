{% from slspath + "/map.jinja" import salt_master with context %}

salt_master:
  pkg.installed:
    - pkgs:
{% for package in salt_master.packages %}
      - {{ package }}
{% endfor %}
  service.running:
    - name: {{ salt_master.service }}
    - enable: True
