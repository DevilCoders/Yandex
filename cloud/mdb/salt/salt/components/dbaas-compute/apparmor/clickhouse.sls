/etc/osquery.tag:
    file.managed:
        - contents: ycloud-mdb-ch-config
        - mode: '0644'
        - require_in:
            - pkg: clickhouse-security-pkgs

old-clickhouse-security-pkgs:
    pkg.purged:
        - name: osquery-ycloud-mdb-ch-config
        - require_in:
            - file: /etc/osquery.tag

clickhouse-security-pkgs:
    pkg.installed:
        - pkgs:
            - osquery-yandex-generic-config: 1.1.1.66
            - apparmor-ycloud-mdb-ch-prof: 1.1.33
        - require:
            - pkg: apparmor
            - pkg: old-clickhouse-security-pkgs

apparmor-complain:
    cmd.wait:
        - name: 'aa-complain /etc/apparmor.d/usr.bin.clickhouse'
        - watch:
            - pkg: clickhouse-security-pkgs
