Install NTP package:
  pkg.installed:
    - name: ntp

Provide NTP config file:
  file.managed:
    - name: /etc/ntp.conf
    - source: salt://ntp/ntp.conf

Enable NTP service:
  service.running:
    - name: ntp
    - enable: True
