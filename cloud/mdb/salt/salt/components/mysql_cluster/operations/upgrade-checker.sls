{% from "components/mysql/map.jinja" import mysql with context %}
include:
    - components.mysql.mysql-shell-install

upgrade-checker:
  cmd.run:
    - name: |
        /usr/local/yandex/mysql_upgrade_checker.py \
        --target-version="{{ salt['pillar.get']('data:versions:mysql:minor_version') }}"
    - require:
      - pkg: upgrade-checker-packages
      - file: /usr/local/yandex/mysql_upgrade_checker.py
