{% set data = salt['pillar.get']("ubic-restarts") %}
{% if data %}
/etc/monitoring/ubic-watchdog-counter.conf:
  file.managed:
    - source: salt://{{ slspath }}/ubic-watchdog-counter.tmpl
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - dir_mode: 0755
    - template: jinja
    - context:
      params: {{ data }}
{% endif %}

