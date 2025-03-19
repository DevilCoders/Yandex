{% set default_iface = salt['grains.get']('default_iface','eth0') %}
packages:
  cluster:
    - yandex-media-common-lxd-bootstrap
    - yandex-internal-root-ca

walle_enabled: False

lxd:
  images: []
  projects:
    - name: bootstrap
      config:
        environment.LXD_CONTAINER_DOMAIN: .yandex.net
      devices:
        eth0:
          nictype: bridged
          parent: br0
          type: nic
    - name: bootstrap-bionic
      config:
        
    - name: bootstrap-precise
      config:
        environment.LXD_CONTAINER_DOMAIN: .yandex.net
      devices:
        eth0:
          nictype: bridged
          parent: br0
          type: nic
    - name: mem_small
      config:
        limits.memory: 16GB
    - name: mem_xsmall
      config:
        limits.memory: 8GB
    - name: mem_medium
      config:
        limits.memory: 32GB
    - name: mem_large
      config:
        limits.memory: 64GB
  init:
    default_volume_size: 100GB
    backend: lvm
    backend_opts: "lvm.use_thinpool=false"
    pool: lxd
  profiles: salt://units/dom0-common/profiles

iface_ip_ignored_interfaces: "lo|docker|dummy|vlan688|vlan700"
