mysql-grants-config:
  file.managed:
    - name: /etc/mysql-grants.conf
    - source: salt://{{ slspath }}/files/etc/mysql-grants.conf
