{% set pins_for_pgdg = ['postgresql-common', 'postgresql-client-common'] %}
{% set distrib = salt['grains.get']('oscodename') %}
{% if salt.pillar.get('data:dist:pgdg:absent') %}
pgdg:
    pkgrepo.absent:
        - name: 'deb http://mirror.yandex.ru/mirrors/postgresql {{ distrib }}-pgdg main'
        - file: /etc/apt/sources.list.d/pgdg.list
        - keyid: ACCC4CF8
        - require_in:
            - cmd: repositories-ready

{% for pkg in pins_for_pgdg %}
/etc/apt/preferences.d/42-pin-{{ pkg }}:
    file.managed:
        - contents: |
            Package: {{ pkg }}
            Pin: release o=mdb-bionic-secure
            Pin-Priority: 500
        - require_in:
            - pkgrepo: mdb-bionic-stable-arch
{% endfor %}
{% else %}
pgdg:
    pkgrepo.managed:
        - refresh: false
        - onchanges_in:
            - cmd: repositories-ready
        - name: 'deb http://mirror.yandex.ru/mirrors/postgresql {{ distrib }}-pgdg main'
        - file: /etc/apt/sources.list.d/pgdg.list
        - key_url: salt://components/repositories/apt/pgdg/ACCC4CF8.asc
        - clean_file: true

{% for pkg in pins_for_pgdg %}
/etc/apt/preferences.d/42-pin-{{ pkg }}:
    file.absent
{% endfor %}
{% endif %}
