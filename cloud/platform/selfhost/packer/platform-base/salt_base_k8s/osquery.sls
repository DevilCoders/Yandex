# Prerequisites
auditd:
  pkg.purged

systemd-journald-audit.socket:
  service.masked

systemd-journald.service:
  service.running:
    - watch:
       - systemd-journald-audit.socket

# Network Access
#check network access to os.sec.yandex.net:
#  module.run:
#    - name: network.connect
#    - host: os.sec.yandex.net
#    - port: 443

# Install Packages
osquery and config:
  pkg.installed:
    - pkgs:
      - osquery-vanilla: 3.3.0
      - osquery-ycloud-svc-generic-config: 1.2.0
