{% from slspath + "/map.jinja" import agent with context %}

agent_packages:
  pkg.installed:
    - pkgs:
    {%- for pkg in agent.packages %}
      - {{ pkg }}
    {%- endfor %}

agent_service_reconfigure:
  service.running:
    - name: icecream-agent
    - enable: True
    - watch:
      - file: /etc/yandex/icecream-agent/*

