/etc/osquery.tag:
    file.managed:
        - contents: ycloud-mdb-greenplum
        - mode: '0644'
        - require_in:
            - pkg: greenplum-security-pkgs

old-greenplum-security-pkgs:
    pkg.purged:
        - name: osquery-ycloud-mdb-greenplum-config
        - require_in:
            - file: /etc/osquery.tag

greenplum-security-pkgs:
    pkg.installed:
        - pkgs:
            - osquery-yandex-generic-config: 1.1.1.66
{% if salt.pillar.get('data:dbaas_compute:apparmor:source', 'package') == 'package' %}
            - apparmor-ycloud-mdb-greenplum-prof: 1.0.45
{% endif %}
        - require:
            - pkg: apparmor
            - pkg: old-greenplum-security-pkgs
        - require_in:
            - service: greenplum-service

{% if salt.pillar.get('data:dbaas_compute:apparmor:source', 'package') == 'salt' %}
apparmor-ycloud-mdb-greenplum-prof:
    pkg.purged

/etc/apparmor.d/opt.greenplum-db.bin.postgres:
    file.managed:
        - source: salt://components/apparmor-profiles/apparmor-ycloud-mdb-greenplum-prof/profile/opt.greenplum-db.bin.postgres
        - mode: '0644'
        - user: root
        - group: root
        - require:
            - pkg: apparmor
            - pkg: apparmor-ycloud-mdb-greenplum-prof

/etc/apparmor.d/pxf.unit:
    file.managed:
        - source: salt://components/apparmor-profiles/apparmor-ycloud-mdb-greenplum-prof/profile/pxf.unit
        - mode: '0644'
        - user: root
        - group: root
        - require:
            - pkg: apparmor
            - pkg: apparmor-ycloud-mdb-greenplum-prof
{% endif %}

apparmor-complain-greenplum:
    cmd.run:
        - name: '{{ salt.pillar.get('data:dbaas_compute:apparmor:greenplum_mode', 'aa-complain') }} /etc/apparmor.d/opt.greenplum-db.bin.postgres'
        - onchanges:
{% if salt.pillar.get('data:dbaas_compute:apparmor:source', 'package') == 'salt' %}
            - file: /etc/apparmor.d/abstractions
            - file: /etc/apparmor.d/opt.greenplum-db.bin.postgres
            - file: /etc/apparmor.d/tunables
        - require:
            - file: /etc/osquery.tag
{% else %}
            - pkg: greenplum-security-pkgs
{% endif %}

apparmor-complain-pxf:
    cmd.run:
        - name: '{{ salt.pillar.get('data:dbaas_compute:apparmor:greenplum_mode', 'aa-complain') }} /etc/apparmor.d/pxf.unit'
        - onchanges:
{% if salt.pillar.get('data:dbaas_compute:apparmor:source', 'package') == 'salt' %}
            - file: /etc/apparmor.d/abstractions
            - file: /etc/apparmor.d/pxf.unit
            - file: /etc/apparmor.d/tunables
        - require:
            - file: /etc/osquery.tag
{% else %}
            - pkg: greenplum-security-pkgs
{% endif %}
