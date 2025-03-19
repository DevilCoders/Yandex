yandex-postgresql-stable-all:
    pkgrepo.managed:
        - refresh: false
        - name: 'deb http://dist.yandex.ru/yandex-postgresql stable/all/'
        - file: /etc/apt/sources.list.d/yandex-postgresql-stable.list
        - onchanges_in:
            - cmd: repositories-ready

yandex-postgresql-stable-arch:
    pkgrepo.managed:
        - refresh: false
        - name: 'deb http://dist.yandex.ru/yandex-postgresql stable/$(ARCH)/'
        - file: /etc/apt/sources.list.d/yandex-postgresql-stable.list
        - onchanges_in:
            - cmd: repositories-ready
