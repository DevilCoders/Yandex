generic_netinfra_config:
    tor_bgp_asn: 65401
    border_bgp_asn: 200350
    fv_reflector_bgp_asn: 200350
    common_transport_bgp_community: 13238:35999
    geo_bgp_communities:
        ru-lab1-a: 13238:35701
        ru-lab1-b: 13238:35701
        ru-lab1-c: 13238:35701
        ru-central1-a: 13238:35702
        ru-central1-b: 13238:35701
        ru-central1-c: 13238:35703
    fv_reflectors:
        prod:
          - name: FVRR_SAS
            ipv4_address: 37.140.141.80
          - name: FVRR_VLA
            ipv4_address: 37.140.141.81
          - name: FVRR_MYT
            ipv4_address: 37.140.141.82
        preprod:
          - name: FVRR_SAS
            ipv4_address: 37.140.141.80
          - name: FVRR_VLA
            ipv4_address: 37.140.141.81
          - name: FVRR_MYT
            ipv4_address: 37.140.141.82
        lab:
          - name: HWLAB_FVRR_SAS
            ipv4_address: 37.140.141.95
    borders:
          - name: M9
            ipv4_address: 37.140.141.89
          - name: STD
            ipv4_address: 37.140.141.88
          - name: TEHNO
            ipv4_address: 37.140.141.91
