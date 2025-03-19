'search-kernel-stable-all':
    pkgrepo.managed:
        - refresh: false
        - onchanges_in:
            - cmd: repositories-ready
        - name: 'deb http://search-kernel.dist.yandex.ru/search-kernel/ stable/all/'
        - file: /etc/apt/sources.list.d/search-kernel-stable.list

'search-kernel-stable-arch':
    pkgrepo.managed:
        - refresh: false
        - onchanges_in:
            - cmd: repositories-ready
        - name: 'deb http://search-kernel.dist.yandex.ru/search-kernel/ stable/$(ARCH)/'
        - file: /etc/apt/sources.list.d/search-kernel-stable.list

{% if salt['pillar.get']('yandex:environment', 'prod') == 'dev' %}
'search-kernel-unstable-all':
    pkgrepo.managed:
        - refresh: false
        - onchanges_in:
            - cmd: repositories-ready
        - name: 'deb http://search-kernel.dist.yandex.ru/search-kernel/ unstable/all/'
        - file: /etc/apt/sources.list.d/search-kernel-unstable.list

'search-kernel-unstable-arch':
    pkgrepo.managed:
        - refresh: false
        - onchanges_in:
            - cmd: repositories-ready
        - name: 'deb http://search-kernel.dist.yandex.ru/search-kernel/ unstable/$(ARCH)/'
        - file: /etc/apt/sources.list.d/search-kernel-unstable.list
{% else %}
/etc/apt/sources.list.d/search-kernel-unstable.list:
    file.absent:
        - onchanges_in:
            - cmd: repositories-ready
{% endif %}

