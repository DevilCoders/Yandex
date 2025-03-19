{% set mirror = salt['pillar.get']('data:repository:mirror', 'storage.yandexcloud.net') %}
{% set url = salt['pillar.get']('data:repository:url', 'dataproc/releases/0.2.18') %}

{% if salt['ydputils.is_presetup']() %}
dataproc-repository:
    pkgrepo.managed:
        - humanname: Yandex Cloud Data-Proc repository
        - name: deb [arch=amd64] http://{{ mirror }}/{{ url }} xenial main
        - file: /etc/apt/sources.list.d/yandex-dataproc.list
        - key_url : file:///srv/dataproc.gpg
        - gpgcheck: 1
        - refresh: True

dataproc-repository-priority:
    file.managed:
        - name: /etc/apt/preferences.d/yandex-dataproc
        - contents:
            - 'Package: *'
            - 'Pin: origin {{ mirror }}'
            - 'Pin-Priority: 700'
{% endif %}
