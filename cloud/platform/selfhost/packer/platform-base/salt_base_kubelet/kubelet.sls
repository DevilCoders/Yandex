/lib/systemd/system/kubelet.service:
  file.managed:
    - source: salt://services/kubelet.service

kubelet:
  service.running:
    - name: kubelet.service
    - enable: True
