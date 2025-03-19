yandex-postgresql-testing-all:
    pkgrepo.managed:
        - refresh: false
        - onchanges_in:
            - cmd: repositories-ready
        - name: 'deb http://dist.yandex.ru/yandex-postgresql testing/all/'
        - file: /etc/apt/sources.list.d/yandex-postgresql-testing.list
        - require_in:
            - pkgrepo: yandex-postgresql-stable-all
            - pkgrepo: yandex-postgresql-stable-arch

yandex-postgresql-testing-arch:
    pkgrepo.managed:
        - refresh: false
        - onchanges_in:
            - cmd: repositories-ready
        - name: 'deb http://dist.yandex.ru/yandex-postgresql testing/$(ARCH)/'
        - file: /etc/apt/sources.list.d/yandex-postgresql-testing.list
        - require_in:
            - pkgrepo: yandex-postgresql-stable-all
            - pkgrepo: yandex-postgresql-stable-arch
