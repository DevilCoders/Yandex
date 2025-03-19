/etc/osquery.tag:
    file.managed:
        - contents: ycloud-mdb-pg-config
        - mode: '0644'
        - require_in:
            - pkg: postgresql-security-pkgs

old-postgresql-security-pkgs:
    pkg.purged:
        - name: osquery-ycloud-mdb-pg-config
        - require_in:
            - file: /etc/osquery.tag

postgresql-security-pkgs:
    pkg.installed:
        - pkgs:
            - osquery-yandex-generic-config: 1.1.1.66
{% if salt.pillar.get('data:dbaas_compute:apparmor:source', 'package') == 'package' %}
            - apparmor-ycloud-mdb-pg-prof: 1.1.38
{% endif %}
        - require:
            - pkg: apparmor
            - pkg: old-postgresql-security-pkgs
        - require_in:
            - service: postgresql-service

{% if salt.pillar.get('data:dbaas_compute:apparmor:source', 'package') == 'salt' %}
apparmor-ycloud-mdb-pg-prof:
    pkg.purged

/etc/apparmor.d/usr.lib.postgresql.bin.postgres:
    file.managed:
        - source: salt://components/apparmor-profiles/apparmor-ycloud-mdb-pg-prof/profile/usr.lib.postgresql.bin.postgres
        - mode: '0644'
        - user: root
        - group: root
        - require:
            - pkg: apparmor
            - pkg: apparmor-ycloud-mdb-pg-prof

/etc/apparmor.d/yc-mdb-pg:
    file.managed:
        - source: salt://components/apparmor-profiles/apparmor-ycloud-mdb-pg-prof/profile/yc-mdb-pg
        - mode: '0644'
        - user: root
        - group: root
        - require:
            - pkg: apparmor
            - pkg: apparmor-ycloud-mdb-pg-prof
{% endif %}

apparmor-complain:
    cmd.run:
        - name: '{{ salt.pillar.get('data:dbaas_compute:apparmor:mode', 'aa-complain') }} /etc/apparmor.d/usr.lib.postgresql.bin.postgres'
        - onchanges:
{% if salt.pillar.get('data:dbaas_compute:apparmor:source', 'package') == 'salt' %}
            - file: /etc/apparmor.d/abstractions
            - file: /etc/apparmor.d/tunables
            - file: /etc/apparmor.d/usr.lib.postgresql.bin.postgres
            - file: /etc/apparmor.d/yc-mdb-pg
        - require:
            - file: /etc/osquery.tag
{% else %} 
            - pkg: postgresql-security-pkgs
{% endif %}
