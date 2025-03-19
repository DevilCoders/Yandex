/etc/default/logstat:
  file.managed:
    - contents: |
        DAEMON_OPTS="/var/log/nginx-access.log -o telegraf -p 8125 -f LOG_FORMAT_RT"

logstat:
  service.running:
    - enable: True
  pkg.installed:
    - name: logstat
    - version: '>=3.0.0'
