/etc/osquery.tag:
    file.managed:
        - contents: ycloud-mdb-mg-config
        - mode: '0644'
        - require_in:
            - pkg: mongodb-security-pkgs

old-mongodb-security-pkgs:
    pkg.purged:
        - name: osquery-ycloud-mdb-mg-config
        - require_in:
            - file: /etc/osquery.tag

mongodb-security-pkgs:
    pkg.installed:
        - pkgs:
            - osquery-yandex-generic-config: 1.1.1.66
            - apparmor-ycloud-mdb-mg-prof: 1.1.17
        - require:
            - pkg: apparmor
            - pkg: old-mongodb-security-pkgs

apparmor-complain:
    cmd.wait:
        - name: 'aa-complain /etc/apparmor.d/usr.bin.mongod'
        - watch:
            - pkg: mongodb-security-pkgs
