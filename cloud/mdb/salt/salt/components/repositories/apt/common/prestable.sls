common-prestable-all:
    pkgrepo.managed:
        - refresh: false
        - onchanges_in:
            - cmd: repositories-ready
        - name: deb http://dist.yandex.ru/common prestable/all/
        - file: /etc/apt/sources.list.d/common-prestable.list
        - require_in:
            - pkgrepo: common-stable-all
            - pkgrepo: common-stable-arch

common-prestable-arch:
    pkgrepo.managed:
        - refresh: false
        - onchanges_in:
            - cmd: repositories-ready
        - name: deb http://dist.yandex.ru/common prestable/$(ARCH)/
        - file: /etc/apt/sources.list.d/common-prestable.list
        - require_in:
            - pkgrepo: common-stable-all
            - pkgrepo: common-stable-arch
