/var/lib/kubelet/config.yaml:
  file.managed:
    - makedirs: True
    - source: salt://kubelet/config.yaml

/lib/systemd/system/kubelet.service:
  file.managed:
    - source: salt://kubelet/kubelet.service

kubelet:
  service.running:
    - name: kubelet.service
    - enable: True
