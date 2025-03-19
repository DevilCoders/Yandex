# Prerequisites
auditd:
  pkg.purged

systemd-journald-audit.socket:
  service.masked

systemd-journald.service:
  service.running:
    - watch:
       - systemd-journald-audit.socket

fill general osquery tag value:
  file.managed:
    - name: /etc/osquery.tag
    - contents:
      - 'ycloud-svc-tools-cloud'

# Install Packages
osquery and config:
  pkg.installed:
    - pkgs:
      - osquery-vanilla: 4.8.0.1
      - osquery-yandex-generic-config: 1.1.1.59

