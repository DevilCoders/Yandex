{% set unit = "postgres" %}

/root/.postgresql/root.crt:
  file.managed:
    - source: salt://{{ slspath }}/files/root.crt
    - user: root
    - group: root
    - mode: 644
    - makedirs: True

/usr/local/share/ca-certificates/root.crt:
  file.managed:
    - source: salt://{{ slspath }}/files/root.crt
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
update-ca-certificates:
  cmd.run:
    - onchanges:
      - file: /usr/local/share/ca-certificates/root.crt
