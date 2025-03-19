/etc/odbc.ini:
  file.managed:
    - source: salt://{{ slspath }}/files/odbc.ini
    - template: jinja
    - user: root
    - group: root
    - mode: 0644
    - dir_mode: 0755

/etc/odbcinst.ini:
  file.managed:
    - source: salt://{{ slspath }}/files/odbcinst.ini
    - user: root
    - group: root
    - mode: 0644
    - dir_mode: 0755

/tmp/tableau/config.json:
   file.managed:
     - source: salt://{{ slspath }}/files/config.json
     - user: root
     - group: root
     - mode: 0644
     - dir_mode: 0755

/tmp/tableau/registration.json:
   file.managed:
     - source: salt://{{ slspath }}/files/registration.json
     - user: root
     - group: root
     - mode: 0644
     - dir_mode: 0755
