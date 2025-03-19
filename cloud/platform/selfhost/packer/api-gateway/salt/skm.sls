/lib/systemd/system/skm.service:
  file.managed:
    - source: salt://skm/skm.service

skm.service:
  service.enabled