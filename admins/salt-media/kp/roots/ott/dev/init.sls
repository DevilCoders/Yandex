/usr/local/bin/sandbox_install.sh:
  file.managed:
    - source: salt://{{slspath}}/files/sandbox_install.sh
    - user: root
    - group: root
    - mode: 0755
