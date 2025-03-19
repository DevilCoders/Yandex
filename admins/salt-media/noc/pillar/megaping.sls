db_host: c-mdb9i7cm2bn6hm4hgoce.rw.db.yandex.net

sec: {{ salt.yav.get('sec-01dwsjyw2xm62apkqzzs580y4h') | json }}


megapings:
  megaping-rad:
    instances:
      check_80:
        zmap_type: check_recalc
        smart_decrease: false
        check_type: tcp_80
        interval: 600
        namespace: nszmap
      check_icmp:
        zmap_type: check_recalc
        smart_decrease: true
        check_type: icmp
        interval: 50
        rate: 10000
        namespace: nszmap
      discovery_icmp:
        zmap_type: discovery
        check_type: icmp
        interval: 3600
        setpoint: 1000
        rate: 100000
        namespace: nszmap
      discovery_tcp_80:
        zmap_type: discovery
        check_type: tcp_80
        interval: 7200
        setpoint: 500
        namespace: nszmap
    namespaces:
      - name: nszmap
        iface: nsiface
        ns_iface: eth0
        net: 192.168.192.0/30
  kiv-megaping1:
    instances:
      check_icmp:
        zmap_type: check
        check_type: icmp
        interval: 120
      check_80:
        zmap_type: check
        check_type: tcp_80
        interval: 120
    zmap_args:
      interface: ens2.695
  sas-megaping01:
    instances:
      check_icmp:
        zmap_type: check
        check_type: icmp
        interval: 120
      check_80:
        zmap_type: check
        check_type: tcp_80
        interval: 120
  man-megaping01:
    instances:
      check_icmp:
        zmap_type: check
        check_type: icmp
        interval: 120
      check_80:
        zmap_type: check
        check_type: tcp_80
        interval: 120
  vla-megaping01:
    instances:
      check_icmp:
        zmap_type: check
        check_type: icmp
        interval: 120
      check_80:
        zmap_type: check
        check_type: tcp_80
        interval: 120
  vlx-megaping00:
    instances:
      check_icmp:
        zmap_type: check
        check_type: icmp
        interval: 120
      check_80:
        zmap_type: check
        check_type: tcp_80
        interval: 120
  vlx-megaping01:
    instances:
      check_icmp:
        zmap_type: check
        check_type: icmp
        interval: 120
      check_80:
        zmap_type: check
        check_type: tcp_80
        interval: 120
  megaping-m9:
    check_icmp:
      zmap_type: check
      check_type: icmp
      interval: 120
    check_80:
      zmap_type: check
      check_type: tcp_80
      interval: 120
    zmap_args:
      interface: eth0.695
  vla-megaping02:
    instances:
      check_80:
        zmap_type: check
        check_type: tcp_80
        interval: 120
        namespace: megaping
      check_icmp:
        zmap_type: check
        check_type: icmp
        interval: 120
        namespace: megaping
    namespaces:
      - name: megaping
        iface: v-eth0
        net: 10.0.0.0/24
    zmap_args:
      interface: v-eth0
  sas-megaping02:
    instances:
      check_80:
        zmap_type: check
        check_type: tcp_80
        interval: 120
        namespace: megaping
      check_icmp:
        zmap_type: check
        check_type: icmp
        interval: 120
        namespace: megaping
    namespaces:
      - name: megaping
        iface: v-eth0
        net: 10.0.0.0/24
    zmap_args:
      interface: v-eth0
  man-megaping02:
    instances:
      check_80:
        zmap_type: check
        check_type: tcp_80
        interval: 120
        namespace: megaping
      check_icmp:
        zmap_type: check
        check_type: icmp
        interval: 120
        namespace: megaping
    namespaces:
      - name: megaping
        iface: v-eth0
        net: 10.0.0.0/24
    zmap_args:
      interface: v-eth0
  myt-megaping02:
    instances:
      check_80:
        zmap_type: check
        check_type: tcp_80
        interval: 120
        namespace: megaping
      check_icmp:
        zmap_type: check
        check_type: icmp
        interval: 120
        namespace: megaping
    namespaces:
      - name: megaping
        iface: v-eth0
        net: 10.0.0.0/24
    zmap_args:
      interface: v-eth0
  vla-lab-megaping02:
    instances:
      check_80:
        zmap_type: check
        check_type: tcp_80
        interval: 120
        namespace: megaping
      check_icmp:
        zmap_type: check
        check_type: icmp
        interval: 120
        namespace: megaping
    namespaces:
      - name: megaping
        iface: v-eth0
        net: 10.0.0.0/24
    zmap_args:
      interface: v-eth0
