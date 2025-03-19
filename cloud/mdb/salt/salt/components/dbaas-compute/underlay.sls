/etc/networkd-dispatcher/routable.d/50-ifup-hooks:
    file.managed:
        - source: salt://{{ slspath }}/conf/netplan-underlay-post-up.sh
        - mode: 755

underlay-mtu-override:
    cmd.run:
        - name: /etc/networkd-dispatcher/routable.d/50-ifup-hooks
        - onchanges:
            - file: /etc/networkd-dispatcher/routable.d/50-ifup-hooks
