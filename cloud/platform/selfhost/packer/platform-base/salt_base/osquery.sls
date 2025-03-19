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

fill general osquery tag value:
  file.managed:
    - name: /etc/osquery.tag
    - contents:
      - 'ycloud-svc-generic-config'

# Install Packages
osquery and config:
  pkg.installed:
    - pkgs:
      - osquery-vanilla: 3.3.1.5
      - osquery-yandex-generic-config: 1.1.0.18

