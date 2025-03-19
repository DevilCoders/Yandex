upgrade-checker-packages:
    pkg.installed:
        - pkgs:
            - mysql-shell: '8.0.26-57c5603f6'

/usr/local/yandex/mysql_upgrade_checker.py:
    file.managed:
        - source: salt://{{ slspath }}/conf/mysql_upgrade_checker.py
        - user: root
        - group: root
        - mode: 755
