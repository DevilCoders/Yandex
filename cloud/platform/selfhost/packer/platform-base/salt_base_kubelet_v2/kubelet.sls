/lib/systemd/system/kubelet.service:
  file.managed:
    - source: salt://services/kubelet.service
