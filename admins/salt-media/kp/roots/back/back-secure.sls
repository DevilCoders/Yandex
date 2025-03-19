/etc/yandex/kino-kp-api/jkp_back_key:
  file.managed:
    - user: root
    - group: root
    - mode: 0640
    - makedirs: True
    - contents: {{ pillar['jkp_back_key'] | json }}