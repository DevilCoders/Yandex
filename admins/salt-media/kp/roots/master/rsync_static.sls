rsyncd_static:
  file.managed:
    - name: /etc/rsync_static.sh
    - source: salt://{{slspath}}/files/rsync_static.sh
    - mode: 640
    - user: root
    - group: www-data
    - template: jinja
