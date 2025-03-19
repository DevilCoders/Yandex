'common-stable-all':
    pkgrepo.managed:
        - refresh: false
        - onchanges_in:
            - cmd: repositories-ready
        - name: 'deb http://dist.yandex.ru/common stable/all/'
        - file: /etc/apt/sources.list.d/common-stable.list
        - order: 2 

'common-stable-arch':
    pkgrepo.managed:
        - refresh: false
        - onchanges_in:
            - cmd: repositories-ready
        - name: 'deb http://dist.yandex.ru/common stable/$(ARCH)/'
        - file: /etc/apt/sources.list.d/common-stable.list
        - order: 2
