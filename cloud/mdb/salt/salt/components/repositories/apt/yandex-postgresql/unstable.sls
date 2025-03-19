yandex-postgresql-unstable-all:
    pkgrepo.managed:
        - refresh: false
        - onchanges_in:
            - cmd: repositories-ready
        - name: 'deb http://dist.yandex.ru/yandex-postgresql unstable/all/'
        - file: /etc/apt/sources.list.d/yandex-postgresql-unstable.list
        - require_in:
            - pkgrepo: yandex-postgresql-stable-all
            - pkgrepo: yandex-postgresql-stable-arch

yandex-postgresql-unstable-arch:
    pkgrepo.managed:
        - refresh: false
        - onchanges_in:
            - cmd: repositories-ready
        - name: 'deb http://dist.yandex.ru/yandex-postgresql unstable/$(ARCH)/'
        - file: /etc/apt/sources.list.d/yandex-postgresql-unstable.list
        - require_in:
            - pkgrepo: yandex-postgresql-stable-all
            - pkgrepo: yandex-postgresql-stable-arch
