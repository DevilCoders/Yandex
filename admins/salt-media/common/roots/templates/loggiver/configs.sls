{% from slspath + "/map.jinja" import loggiver with context %}

{{ loggiver.dpath }}:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - source: {{ loggiver.spath }}
    - watch_in:
      - service: {{ loggiver.service }}
