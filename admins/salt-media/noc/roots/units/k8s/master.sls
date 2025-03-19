include:
  - units.k8s.main-packages
  - units.k8s.secrets

/root/.bashrc:
  file.append:
    - text:
      - "export KUBECONFIG=/etc/kubernetes/admin.conf"
      - "source <(kubectl completion bash)"

/root/init:
  file.recurse:
    - source: salt://{{ slspath }}/files/init

/usr/sbin/etcd-backup.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/sbin/etcd-backup.sh
    - mode: 755
/etc/cron.d/etcd-backup:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/cron.d/etcd-backup

# copy from arcadia/noc/admin/argocd/apps/argocd
/root/argocd:
  file.recurse:
    - source: salt://{{ slspath }}/files/argocd
    - template: jinja
    - makedirs: True

# https://argo-cd.readthedocs.io/en/stable/getting_started/#2-download-argo-cd-cli
/usr/sbin/argocd:
  file.managed:
    - source: https://github.com/argoproj/argo-cd/releases/download/v2.4.7/argocd-linux-amd64
    - skip_verify: True
    - mode: 755

# TODO: unpack
/root/helm-v3.9.2-linux-amd64.tar.gz:
  file.managed:
    - source: https://get.helm.sh/helm-v3.9.2-linux-amd64.tar.gz
    - source_hash: https://get.helm.sh/helm-v3.9.2-linux-amd64.tar.gz.sha256sum
    - source_hash_name: helm-v3.9.2-linux-amd64.tar.gz
    - mode: 755
