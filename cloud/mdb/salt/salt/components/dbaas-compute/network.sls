/etc/network/interfaces:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/interfaces
        - mode: 644

/usr/local/yandex/eth0_post_up.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/eth0_post_up.sh
        - mode: 755

/usr/local/yandex/eth1_post_up.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/eth1_post_up.sh
        - mode: 755
