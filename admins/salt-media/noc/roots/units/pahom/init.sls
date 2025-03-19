/etc/nginx/sites-enabled/pahom.conf:
  file.managed:
    - source: salt://{{slspath}}/files/etc/nginx/sites-enabled/pahom.conf
    - template: jinja
    - makedirs: True

/etc/conf.d/pahom:
  file.managed:
    - source: salt://{{slspath}}/files/etc/conf.d/pahom
    - template: jinja
    - user: root
    - group: root
    - mode: 400
    - makedirs: True

