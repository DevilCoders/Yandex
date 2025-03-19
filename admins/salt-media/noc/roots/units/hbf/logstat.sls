logstat:
  pkg.installed:
    - version: '>=3.0.0'
  service.running:
    - enable: True
    - watch:
      - file: /etc/default/logstat

/etc/default/logstat:
  file.managed:
    - contents: |
        # Modified by salt from {{slspath}}
        DAEMON_OPTS="/var/log/nginx/tskv.log -o telegraf -p 8125 -dp 8126 -f LOG_FORMAT_HBF"
