yandex-postgresql-prestable-all:
    pkgrepo.managed:
        - refresh: false
        - onchanges_in:
            - cmd: repositories-ready
        - name: 'deb http://dist.yandex.ru/yandex-postgresql prestable/all/'
        - file: /etc/apt/sources.list.d/yandex-postgresql-prestable.list
        - require_in:
            - pkgrepo: yandex-postgresql-stable-all
            - pkgrepo: yandex-postgresql-stable-arch

yandex-postgresql-prestable-arch:
    pkgrepo.managed:
        - refresh: false
        - onchanges_in:
            - cmd: repositories-ready
        - name: 'deb http://dist.yandex.ru/yandex-postgresql prestable/$(ARCH)/'
        - file: /etc/apt/sources.list.d/yandex-postgresql-prestable.list
        - require_in:
            - pkgrepo: yandex-postgresql-stable-all
            - pkgrepo: yandex-postgresql-stable-arch
