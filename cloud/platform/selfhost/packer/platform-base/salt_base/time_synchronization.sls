Config for systemd-timesyncd.service:
  file.managed:
    - name: /etc/systemd/timesyncd.conf
    - source: salt://ntp/timesyncd.conf

Enable systemd timesyncd service:
  service.running:
    - name: systemd-timesyncd.service
    - enable: True
