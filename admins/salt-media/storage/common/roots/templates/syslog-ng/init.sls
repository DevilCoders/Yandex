{% for source, files in pillar.get('syslog-ng-files', {}).items() %}
{% for file in files %}

{{ file }}:
  yafile.managed:
    - source: {{ source }}{{ file }}
    - user: root
    - group: root
    - mode: 644
    - watch_in:
      - service: syslog-ng

{{ file | replace("conf-available", "conf-enabled")}}:
  file.symlink:
    - target: {{ file }}

{% endfor %}
{% endfor %}

syslog-ng:
  service:
    - running
    - reload: True
