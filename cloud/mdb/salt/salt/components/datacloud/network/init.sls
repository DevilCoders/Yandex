netplan:
    pkg.purged

netplan.io:
    pkg.purged

ifupdown:
    pkg.installed

/etc/network/interfaces.d:
  file.directory:
    - clean: True

/etc/network/interfaces:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/interfaces
        - mode: 644

/etc/hosts:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/hosts
        - mode: 644

reconfigure-ens5:
    cmd.wait:
        - name: ifdown ens5 && ifup ens5
        - watch:
            - file: /etc/network/interfaces
