/usr/local/bin/generate-network-yanetconfig.sh:
  file.managed:
    - source: salt:///units/ya-netconfig/files/generate-network-yanetconfig.sh
    - mode: 755
    - user: root
    - group: root

/etc/network/Makefile:
  file.managed:
    - source: salt:///units/ya-netconfig/files/Makefile
    - mode: 644
    - user: root
    - group: root
    - require:
        - file: /usr/local/bin/generate-network-yanetconfig.sh

regenerate-interfaces:
  cmd.run:
    - name: make -C /etc/network
    - unless: grep -q ya-slb-tun /etc/network/interfaces
    - require:
        - file: /etc/network/Makefile
