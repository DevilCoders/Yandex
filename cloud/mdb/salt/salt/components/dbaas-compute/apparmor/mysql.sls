/etc/osquery.tag:
    file.managed:
        - contents: ycloud-mdb-mysql-config
        - mode: '0644'
        - require_in:
            - pkg: mysql-security-pkgs

old-mysql-security-pkgs:
    pkg.purged:
        - name: osquery-ycloud-mdb-mysql-config
        - require_in:
            - file: /etc/osquery.tag

mysql-security-pkgs:
    pkg.installed:
        - pkgs:
            - osquery-yandex-generic-config: 1.1.1.66
            - apparmor-ycloud-mdb-mysql-prof: 1.0.21
        - require:
            - pkg: apparmor
            - pkg: old-mysql-security-pkgs
        - require_in:
            - service: mysql

apparmor-complain:
    cmd.wait:
        - name: 'aa-complain /etc/apparmor.d/usr.sbin.mysql'
        - watch:
            - pkg: mysql-security-pkgs
