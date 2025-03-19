percona-toolkit-for-pt-kill-state:
  pkg.installed:
    - pkgs:
      - percona-toolkit

{% for state, times in salt['pillar.get']('pt-kill:systemd:busy-time').items() %}
{% for time in times %}
pt-kill-{{ time }}:
  file.managed:
    - name: /lib/systemd/system/pt-kill-{{ time }}.service
    - source: salt://{{ slspath }}/files/pt-kill.service
    - template: jinja
    - context:
      time: {{ time }}
  {% if state == 'enable' %}
  service.running:
    - restart: True
    - enable: True
  {% else %}
  service.dead:
    - restart: False
    - enable: False
  {% endif %}
{% endfor %}
{% endfor %}
