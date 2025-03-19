{% set mirror = salt['pillar.get']('data:repository:mirror', 'storage.yandexcloud.net') %}
{% set url = salt['pillar.get']('data:repository:url', 'dataproc/ci/trunk/76-c457b769e6254335') %}

{% if salt['ydputils.is_presetup']() %}
dataproc-repository:
    pkgrepo.managed:
        - humanname: Yandex Cloud Data Proc repository
        - name: deb [arch=amd64] http://{{ mirror }}/{{ url }} focal main
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

{% set ppa_list = salt['ydputils.get_ppa_repositories']() %}
{% if ppa_list != [] %}
ppa-repositories:
{% for ppa in ppa_list %}
    pkgrepo.managed:
        - ppa: {{ ppa }}
        - refresh: False
{% endfor %}
{% endif %}