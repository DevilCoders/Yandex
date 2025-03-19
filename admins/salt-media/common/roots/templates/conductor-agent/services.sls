{% from slspath + "/map.jinja" import conductor_agent with context %}

conductor_agent_packages:
  pkg.installed:
    - pkgs:
    {%- for pkg in conductor_agent.packages %}
      - {{ pkg }}
    {%- endfor %}

  service.running:
    - name: {{ conductor_agent.service }}
    - enable: True
