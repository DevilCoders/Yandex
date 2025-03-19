/usr/share/keyrings/kubernetes-archive-keyring.gpg:
  file.managed:
    - source: salt://{{ slspath }}/files/kubernetes-archive-keyring.gpg

/etc/apt/sources.list.d/kubernetes.list:
  file.managed:
    - source: salt://{{ slspath }}/files/kubernetes.list

/etc/default/config-caching-dns:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/default/config-caching-dns

apt-transport-https:
  pkg.installed:
    - refresh: True
    - allow_updates: True

ipvsadm:
  pkg.installed:
    - refresh: True

/etc/ndppd.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/ndppd.conf
    - template: jinja

ndppd:
  pkg.installed:
    - refresh: True
  service.running:
    - enable: True

/etc/crictl.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/crictl.yaml

/etc/sysctl.d/99-k8s.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/sysctl.d/99-k8s.conf

containerd:
  pkg.installed:
    - refresh: True

kube:
  pkg.installed:
    - version: {{pillar['kube-version']}}
    - refresh: True
    - hold: True
    - pkgs:
      - kubectl
      - kubelet
      - kubeadm

calico-manager:
  pkg.installed:
    - refresh: True

# https://projectcalico.docs.tigera.io/maintenance/clis/calicoctl/install
/usr/sbin/calicoctl:
  file.managed:
    - source: https://github.com/projectcalico/calico/releases/download/v3.23.3/calicoctl-linux-amd64
    - source_hash: https://github.com/projectcalico/calico/releases/download/v3.23.3/SHA256SUMS
    - source_hash_name: calicoctl-linux-amd64
    - mode: 755
