autoscale-agent-packages:
  yc_pkg.installed:
    - pkgs:
      - yc-autoscale-agent

autoscale-agent-config:
  file.managed:
    - name: /etc/yc/autoscale-agent/config.yaml
    - makedirs: True
    - dir_mode: 755
    - user: root
    - group: root
    - template: jinja
    - source: salt://{{ slspath }}/config.yaml
    - require:
      - yc_pkg: autoscale-agent-packages

autoscale-agent_systemd_unit_drop-in:
  file.managed:
    - name: /etc/systemd/system/yc-autoscale-agent.service.d/10-cpu-memory-limit.conf
    - source: salt://{{ slspath }}/10-cpu_memory_limit.conf
    - template: jinja
    - makedirs: True

autoscale-agent:
  service.running:
    - enable: true
    - name: yc-autoscale-agent
    - watch:
      - file: autoscale-agent_systemd_unit_drop-in
      - file: autoscale-agent-config
      - yc_pkg: autoscale-agent-packages
