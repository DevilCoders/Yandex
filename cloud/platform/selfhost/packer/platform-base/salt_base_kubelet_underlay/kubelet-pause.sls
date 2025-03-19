/lib/systemd/system/kubelet.service:
  file.managed:
    - source: salt://services/kubelet.service

pull pause:
  cmd.run:
    - name: docker pull k8s.gcr.io/pause:3.1

kubelet:
  service.running:
    - name: kubelet.service
    - enable: True
