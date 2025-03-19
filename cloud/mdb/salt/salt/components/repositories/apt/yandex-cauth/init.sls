yandex-cauth-apt-repo:
    pkgrepo.managed:
        - refresh: false
        - name: 'deb http://cauth.dist.yandex.ru/cauth stable/$(ARCH)/'
        - file: /etc/apt/sources.list.d/yandex-cauth.list
        - onchanges_in:
            - cmd: repositories-ready
