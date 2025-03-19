/etc/sysctl.d/60-ipvs_tun.conf:
  file.managed:
    - source: salt://kibana/etc/sysctl.d/60-ipvs_tun.conf
