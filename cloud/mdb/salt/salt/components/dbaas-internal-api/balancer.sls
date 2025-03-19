# Configure network interfaces. Should run only as simple state

/etc/network/interfaces:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/interfaces.conf
        - mode: '0640'
        - user: root
        - group: root
