include: 
  - netinfra.netinfra-generic
  - netinfra.root_dns_servers
  - netinfra.yandex_cloud_prefixes
  - netinfra.yandex_llc_prefixes
  - netinfra.yandex_telecom_prefixes


yc-pinning:
    packages:
    bird: 1.5.0-4build1
    suricata: 3.2-2ubuntu6.39
    yc-rkn-filter-node: 0.1-26.190422
    gobgp: 1.33.2.38

generic_rfn_config:
    local_bgp_asn: 65402
    transport_bgp_commuity: 13238:35160
    rkn_routes_bgp_community: 65000:6600
    whitelist_bgp_community: 65000:6601
    blacklist_bgp_community: 65000:6602
    host_route_bgp_community: 65000:6603
    subnet_route_bgp_community: 65000:6604
    rfn_nginx_loopback_ipv4_address: 185.32.185.65
specific_rfn_config:
    SALT-MINION-1:
        deployment: lab
        rfn_fv_peering_loopback_ipv4_address: 172.16.1.120
        gre_tunnels:
            M9:
                local_tunnel_ipv4_address:  192.168.240.2
                remote_tunnel_ipv4_address: 192.168.240.1
            STD:
                local_tunnel_ipv4_address:  192.168.241.2
                remote_tunnel_ipv4_address: 192.168.241.1
    rfn-hw1a-1.svc.hw1.cloud-lab.yandex.net:
        deployment: lab
        rfn_fv_peering_loopback_ipv4_address: 172.16.1.120
        gre_tunnels:
            M9:
                local_tunnel_ipv4_address:  192.168.240.2
                remote_tunnel_ipv4_address: 192.168.240.1
            STD:
                local_tunnel_ipv4_address:  192.168.241.2
                remote_tunnel_ipv4_address: 192.168.241.1
    rfn-sas1.svc.cloud-preprod.yandex.net:
        deployment: prod
        rfn_fv_peering_loopback_ipv4_address: 172.16.1.121
        gre_tunnels:
            M9:
                local_tunnel_ipv4_address:  192.168.242.2
                remote_tunnel_ipv4_address: 192.168.242.1
            STD:
                local_tunnel_ipv4_address:  192.168.243.2
                remote_tunnel_ipv4_address: 192.168.243.1
    rfn-vla1.svc.cloud-preprod.yandex.net:
        deployment: prod
        rfn_fv_peering_loopback_ipv4_address: 172.16.1.122
        gre_tunnels:
            M9:
                local_tunnel_ipv4_address:  192.168.244.2
                remote_tunnel_ipv4_address: 192.168.244.1
            STD:
                local_tunnel_ipv4_address:  192.168.245.2
                remote_tunnel_ipv4_address: 192.168.245.1
    rfn-myt1.svc.cloud-preprod.yandex.net:
        deployment: prod
        rfn_fv_peering_loopback_ipv4_address: 172.16.1.123
        gre_tunnels:
            M9:
                local_tunnel_ipv4_address:  192.168.246.2
                remote_tunnel_ipv4_address: 192.168.246.1
            STD:
                local_tunnel_ipv4_address:  192.168.247.2
                remote_tunnel_ipv4_address: 192.168.247.1
    rfn-sas1.svc.cloud.yandex.net:
        deployment: prod
        rfn_fv_peering_loopback_ipv4_address: 172.16.1.124
        gre_tunnels:
            M9:
                local_tunnel_ipv4_address:  192.168.248.2
                remote_tunnel_ipv4_address: 192.168.248.1
            STD:
                local_tunnel_ipv4_address:  192.168.249.2
                remote_tunnel_ipv4_address: 192.168.249.1
    rfn-vla1.svc.cloud.yandex.net:
        deployment: prod
        rfn_fv_peering_loopback_ipv4_address: 172.16.1.125
        gre_tunnels:
            M9:
                local_tunnel_ipv4_address:  192.168.250.2
                remote_tunnel_ipv4_address: 192.168.250.1
            STD:
                local_tunnel_ipv4_address:  192.168.251.2
                remote_tunnel_ipv4_address: 192.168.251.1
    rfn-myt1.svc.cloud.yandex.net:
        deployment: prod
        rfn_fv_peering_loopback_ipv4_address: 172.16.1.126
        gre_tunnels:
            M9:
                local_tunnel_ipv4_address:  192.168.252.2
                remote_tunnel_ipv4_address: 192.168.252.1
            STD:
                local_tunnel_ipv4_address:  192.168.253.2
                remote_tunnel_ipv4_address: 192.168.253.1
