'common-unstable-all':
    pkgrepo.managed:
        - refresh: false
        - onchanges_in:
            - cmd: repositories-ready
        - name: 'deb http://dist.yandex.ru/common unstable/all/'
        - file: /etc/apt/sources.list.d/common-unstable.list
        - require_in:
            - pkgrepo: common-stable-all
            - pkgrepo: common-stable-arch

'common-unstable-arch':
    pkgrepo.managed:
        - refresh: false
        - onchanges_in:
            - cmd: repositories-ready
        - name: 'deb http://dist.yandex.ru/common unstable/$(ARCH)/'
        - file: /etc/apt/sources.list.d/common-unstable.list
        - require_in:
            - pkgrepo: common-stable-all
            - pkgrepo: common-stable-arch
