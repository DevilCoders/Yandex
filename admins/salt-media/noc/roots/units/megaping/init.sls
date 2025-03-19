megaping-pkgs:
  pkg:
    - installed
    - pkgs:
      - megaping

/etc/megaping.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/megaping.yaml
    - template: jinja
    - user: root
    - group: root
    - mode: 644

/etc/megaping_db_password:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/megaping_db_password
    - template: jinja
    - user: root
    - group: root
    - mode: 400

/etc/megaping_solomon_password:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/megaping_solomon_password
    - template: jinja
    - user: root
    - group: root
    - mode: 400

megaping:
  service.running:
    - enable: True
    - watch:
        - file: /etc/megaping.yaml
        - file: /etc/megaping_solomon_password
        - file: /etc/megaping_db_password
