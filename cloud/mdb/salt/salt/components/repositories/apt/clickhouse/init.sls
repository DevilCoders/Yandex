clickhouse-repo:
    pkgrepo.managed:
        - name: 'deb https://repo.yandex.ru/clickhouse/deb/stable/ main/'
        - file: /etc/apt/sources.list.d/clickhouse.list
        - key_url: salt://{{ slspath }}/clickhouse.gpg
        - require_in:
            - cmd: repositories-ready
    pkg.installed:
        - pkgs:
            - apt-transport-https
        - require_in:
            - pkgrepo: clickhouse-repo
