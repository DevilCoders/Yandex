{% set unit = 'ipvs_ping' %}

/etc/sysctl.d/10-network-security.conf:
  file.absent:
    - name: /etc/sysctl.d/10-network-security.conf
