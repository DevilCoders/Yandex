include: 
  - netinfra.netinfra-generic


yc-pinning:
    packages:
        gobgp: 1.32.2.32


generic_decap_config:
    local_bgp_asn: 65402
    transport_bgp_community: 13238:35165
    decap_anycast_community: 65000:6700
    hugepage_size: 2048kB
    hugepage_number: 6144
    anycast_route:
        lab:
            ru-lab1-a:
                address: 2a02:6b8:c02:904:0:fa00:0:deca
                prefix:  2a02:6b8:c02:904::/64
        prod:
            ru-central1-a:
                address: 2a02:6b8:c0e:502:0:fa00:0:deca 
                prefix:  2a02:6b8:c0e:502::/64
            ru-central1-b:
                address: 2a02:6b8:c02:902:0:fa00:0:deca 
                prefix:  2a02:6b8:c02:902::/64
            ru-central1-c:
                address: 2a02:6b8:c03:502:0:fa00:0:deca 
                prefix:  2a02:6b8:c03:502::/64
        preprod:
            ru-central1-a:
                address: 2a02:6b8:c0e:503:0:fa00:0:deca 
                prefix:  2a02:6b8:c0e:503::/64
            ru-central1-b:
                address: 2a02:6b8:c02:903:0:fa00:0:deca 
                prefix:  2a02:6b8:c02:903::/64
            ru-central1-c:
                address: 2a02:6b8:c03:503:0:fa00:0:deca 
                prefix:  2a02:6b8:c03:503::/64


specific_decap_config:
    decap-hw1a-1.svc.hw1.cloud-lab.yandex.net:
        deployment: lab
        decap_fv_peering_loopback_ipv4_address: 172.16.1.112
    decap-hw1a-2.svc.hw1.cloud-lab.yandex.net:
        deployment: lab
        decap_fv_peering_loopback_ipv4_address: 172.16.1.113
    decap-vla1.svc.cloud-preprod.yandex.net:
        deployment: preprod
        decap_fv_peering_loopback_ipv4_address: 172.16.1.114
    decap-vla2.svc.cloud-preprod.yandex.net:
        deployment: preprod
        decap_fv_peering_loopback_ipv4_address: 172.16.1.115
    decap-sas1.svc.cloud-preprod.yandex.net:
        deployment: preprod
        decap_fv_peering_loopback_ipv4_address: 172.16.1.116
    decap-sas2.svc.cloud-preprod.yandex.net:
        deployment: preprod
        decap_fv_peering_loopback_ipv4_address: 172.16.1.117
    decap-myt1.svc.cloud-preprod.yandex.net:
        deployment: preprod
        decap_fv_peering_loopback_ipv4_address: 172.16.1.118
    decap-myt2.svc.cloud-preprod.yandex.net:
        deployment: preprod
        decap_fv_peering_loopback_ipv4_address: 172.16.1.119
    decap-vla1.svc.cloud.yandex.net:
        deployment: prod
        decap_fv_peering_loopback_ipv4_address: 172.16.1.96
    decap-vla2.svc.cloud.yandex.net:
        deployment: prod
        decap_fv_peering_loopback_ipv4_address: 172.16.1.97
    decap-sas1.svc.cloud.yandex.net:
        deployment: prod
        decap_fv_peering_loopback_ipv4_address: 172.16.1.98
    decap-sas2.svc.cloud.yandex.net:
        deployment: prod
        decap_fv_peering_loopback_ipv4_address: 172.16.1.99
    decap-myt1.svc.cloud.yandex.net:
        deployment: prod
        decap_fv_peering_loopback_ipv4_address: 172.16.1.100
    decap-myt2.svc.cloud.yandex.net:
        deployment: prod
        decap_fv_peering_loopback_ipv4_address: 172.16.1.101