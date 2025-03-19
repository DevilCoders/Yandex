{% from slspath + "/map.jinja" import loggiver with context %} 

loggiver_service:
  pkg.installed:
    - pkgs:
    {%- for pkg in loggiver.packages %}
      - {{ pkg }}
    {%- endfor %}
  service.running:
    - name: {{ loggiver.service }}
    - enable: True
    - reload: True
