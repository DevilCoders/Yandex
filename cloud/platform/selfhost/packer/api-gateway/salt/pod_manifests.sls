pod_manifests:
  file.managed:
    - user: root
    - group: root
    - mode: '0644'
    - makedirs: True
    - names:
      - /etc/kubelet.d/app.yaml:
        - source: salt://pod_manifests/app.yaml
      - /etc/kubelet.d/infra.yaml:
        - source: salt://pod_manifests/infra.yaml
