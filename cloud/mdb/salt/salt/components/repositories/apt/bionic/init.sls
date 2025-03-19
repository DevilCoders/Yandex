{% if salt.pillar.get('data:dist:bionic:secure') %}
{% set bionic_url = salt.pillar.get('data:dist:bionic:url', 'http://dist.yandex.ru/yandex-cloud-upstream-bionic-secure') %}

'yandex-cloud-upstream-bionic-secure-stable-all':
    pkgrepo.managed:
        - refresh: false
        - name: 'deb {{ bionic_url }} stable/all/'
        - file: /etc/apt/sources.list.d/yandex-cloud-upstream-bionic-secure.list
        - onchanges_in:
            - cmd: repositories-ready

'yandex-cloud-upstream-bionic-secure-stable-arch':
    pkgrepo.managed:
        - refresh: false
        - name: 'deb {{ bionic_url }} stable/$(ARCH)/'
        - file: /etc/apt/sources.list.d/yandex-cloud-upstream-bionic-secure.list
        - onchanges_in:
            - cmd: repositories-ready

/etc/apt/sources.list:
    file.absent

{% else -%}

'ubuntu-bionic-main':
    pkgrepo.managed:
        - refresh: false
        - name: 'deb http://mirror.yandex.ru/ubuntu bionic main restricted universe multiverse'
        - onchanges_in:
            - cmd: repositories-ready

'ubuntu-bionic-security':
    pkgrepo.managed:
        - refresh: false
        - name: 'deb http://mirror.yandex.ru/ubuntu bionic-security main restricted universe multiverse'
        - onchanges_in:
            - cmd: repositories-ready

'ubuntu-bionic-updates':
    pkgrepo.managed:
        - refresh: false
        - name: 'deb http://mirror.yandex.ru/ubuntu bionic-updates main restricted universe multiverse'
        - onchanges_in:
            - cmd: repositories-ready

/etc/apt/sources.list.d/yandex-cloud-upstream-bionic-secure.list:
    file.absent

{% endif -%}
