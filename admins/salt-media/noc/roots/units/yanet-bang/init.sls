/etc/bangd/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/config.yaml
    - template: jinja
    - user: bang
    - group: bang
    - mode: 600
    - makedirs: True

/etc/bangd/ssl/key.bang.yandex-team.ru.pem:
  file.managed:
    - user: bang
    - group: bang
    - mode: 600
    - makedirs: True
    - contents: {{ pillar['sec-crt']['7F001C56BC09F206DCB2B256FB0002001C56BC_private_key'] }}

/etc/bangd/ssl/cert.bang.yandex-team.ru.pem:
  file.managed:
    - user: bang
    - group: bang
    - mode: 600
    - makedirs: True
    - contents: {{ pillar['sec-crt']['7F001C56BC09F206DCB2B256FB0002001C56BC_certificate'] }}
