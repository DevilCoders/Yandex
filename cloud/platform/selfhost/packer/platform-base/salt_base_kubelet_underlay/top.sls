base:
  '*':
    - packages
    - docker
    - kubernetes-creds-daemon
    - kubelet-pause
    - yandex_internal_root_ca
    - compute-metaproxy
    - compute-metaproxy-gce
    - cloud-init-fix
    - clean-gce
    - gce-fix-hostname
    # Network must be the last, otherwise we overwrite NAT64 in resolv.conf
    - network
    - selfdns-client
    - slb
    - osquery