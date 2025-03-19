mysql-wrapper:
  file.managed:
    - name: /usr/local/bin/mysql-wrapper
    - source: salt://{{ slspath }}/files/mysql-wrapper
    - user: root
    - group: root
    - mode: 755
    - makedirs: True
