infrastructure host packages:
  pkg.installed:
  - pkgs:
    - kubelet=1.12.2-00
    - kubernetes-cni=0.6.0-00
    - docker-ce=5:19.03~stop~timeout~yc~3-0~ubuntu-xenial
    - docker-ce-cli=5:19.03~stop~timeout~yc~3-0~ubuntu-xenial
    - containerd.io=1.2.10-3
    - yandex-internal-root-ca
