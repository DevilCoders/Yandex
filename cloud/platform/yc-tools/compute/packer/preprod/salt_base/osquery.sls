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
      - 'ycloud-svc-tools-cloud'

# Install Packages
osquery and config:
  pkg.installed:
    - pkgs:
      - osquery-vanilla: 4.1.1.3
      - osquery-yandex-generic-config: 1.1.1.13

