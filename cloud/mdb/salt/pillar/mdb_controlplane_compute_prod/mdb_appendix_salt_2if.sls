data:
    network:
        interfaces: |
            auto lo
            iface lo inet loopback

            auto eth0
            iface eth0 inet dhcp
            iface eth0 inet6 dhcp
                accept_ra 1
                post-up /sbin/ip -6 route add 2a02:6b8::/32 dev eth0
                post-up /sbin/ip -6 route add 2a0d:d6c0:0::/40 dev eth0

            auto eth1
            iface eth1 inet6 dhcp
                accept_ra 0
                post-up /sbin/ip -6 route add 2a0d:d6c0:200::/40 dev eth1

include:
    - mdb_controlplane_compute_prod.mdb_appendix_salt
