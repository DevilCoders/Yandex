compute-metaproxy_pkg:
  pkg.installed:
    - pkgs:
      - yc-compute-metaproxy

/etc/yc/compute-metaproxy/config.yaml:
  file.managed:
    - source: salt://files/compute-metaproxy-config.yaml

/lib/systemd/system/yc-compute-metaproxy.service:
  file.managed:
    - source: salt://services/yc-compute-metaproxy.service

/etc/modules-load.d/vmw_vsock_virtio_transport.conf:
  file.managed:
    - contents:
      - 'vmw_vsock_virtio_transport'
