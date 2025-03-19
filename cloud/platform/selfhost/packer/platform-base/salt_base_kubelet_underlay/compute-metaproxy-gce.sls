socat package:
  pkg.installed:
  - pkgs:
    - socat

/lib/systemd/system/yc-compute-metaproxy-gce.service:
  file.managed:
    - source: salt://services/yc-compute-metaproxy-gce.service

yc_compute_metaproxy_gce_service:
  service.enabled:
  - name: yc-compute-metaproxy-gce.service

# This fixes the case when slb configuration removes the 127.0.0.1 from lo.
/etc/network/interfaces.d/99_yc_metadata:
  file.managed:
    - source: salt://files/99_yc_metadata