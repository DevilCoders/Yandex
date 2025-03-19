{% set unit = 'dashing-ping' %}

{{ pillar.get('dashing-ping_path', '/usr/local/share/dashing-ping/dashing-ping-new.py') }}:
  file.managed:
    - source: salt://templates/dashing-ping/dashing-ping-new.py
    - mode: 755
    - user: root
    - group: root
    - makedirs: True

{{ pillar.get('dashing-ping_cron', '/etc/cron.d/pre_ping') }}:
  file.managed:
    - source: salt://templates/dashing-ping/pre_ping
    - template: jinja
    - mode: 644
    - user: root
    - group: root
    - makedirs: True

{{ pillar.get('dashing-ping_pre', '/usr/local/share/dashing-ping/pre_ping.py') }}:
  file.managed:
    - source: salt://templates/dashing-ping/pre_ping.py
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
