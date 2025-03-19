platform host packages:
  pkg.installed:
    - pkgs:
      - kubelet=1.16.1-00
      - kubernetes-cni=0.7.5-00
      - docker.io=18.09.7-0ubuntu1~16.04.5

/var/lib/kubelet/config.yaml:
  file.managed:
    - source: salt://kubelet/config.yaml


/lib/systemd/system/kubelet.service:
  file.managed:
    - source: salt://kubelet/kubelet.service

kubelet:
  service.running:
    - name: kubelet.service
    - enable: True
