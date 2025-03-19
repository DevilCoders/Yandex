{% set default_iface = salt['grains.get']('default_iface','eth0') %}
packages:
  filtered:
    - config-media-admins-public-keys
  cluster:
    - yandex-media-common-lxd-bootstrap
    - yandex-internal-root-ca

walle_enabled: True

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
    - name: mds-test
      config:
        environment.LXD_CONDUCTOR_GROUP: elliptics-test-lost
        environment.LXD_YANDEX_ENVIRONMENT: testing
    - name: dev
      config:
        environment.LXD_CONDUCTOR_GROUP: elliptics-dev-lxd
        environment.LXD_YANDEX_ENVIRONMENT: testing
    - name: storage-test
      config:
        environment.LXD_CONDUCTOR_GROUP: storage-lost
        environment.LXD_YANDEX_ENVIRONMENT: testing
    - name: proxy
      config:
        limits.memory: 64GB
    - name: salt
      config:
        limits.memory: 16GB
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
    - name: mds
      config:
        environment.LXD_CONDUCTOR_GROUP: elliptics-lost
        environment.LXD_YANDEX_ENVIRONMENT: production
  init:
    default_volume_size: 100GB
    backend: lvm
    backend_opts: "lvm.use_thinpool=false"
    pool: lxd
  profiles: salt://units/dom0-common/profiles

iface_ip_ignored_interfaces: "lo|docker|dummy|vlan688|vlan700"
