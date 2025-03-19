new template for cloud-init to form /etc/hosts:
  file.managed:
    - name: /etc/cloud/templates/hosts.debian.tmpl
    - source: salt://{{ slspath }}/hosts.debian.tmpl
    - makedirs: True

Remove cloud-init generated network configs:
  file.directory:
    - name: /etc/network/interfaces.d/
    - clean: True

Provide static value (one NIC):
  file.managed:
    - name: /etc/network/interfaces
    - contents: |
        auto lo
        iface lo inet loopback

        auto eth0
        iface eth0 inet dhcp
        iface eth0 inet6 dhcp
          # default route based on RA
          accept_ra 2
          # will not fallback to SLAAC
          autoconf 0
          # below used in /lib/ifupdown/wait-for-ll6.sh
          # to wait ready state for NIC to assign address from DHCP
          ll-attempts 120
          ll-interval 0.5

Sysctl network settings:
  file.managed:
    - name: /etc/sysctl.d/30-network.conf
    - contents: |
        # do not use DAD (speed up assign of link-local address)
        net.ipv6.conf.eth0.accept_dad = 0
        net.ipv6.conf.eth0.dad_transmits = 0

Sysctl network settings - volume 2:
  file.managed:
    - name: /etc/sysctl.d/30-network.conf
    - source: salt://network/net-sysctl.conf
    - template:

Dhcpv6 client settings:
  file.managed:
    - name: /etc/dhcp/dhclient6.conf
    - contents: |
        timeout 10;

Disable cloud-init networking configuration:
  file.managed:
    - name: /etc/cloud/cloud.cfg.d/99-disable-network-config.cfg
    - contents: |
        network:
         config:
          disabled
