install up-to-date SKM binary:
  file.managed:
    - source: https://storage.cloud-preprod.yandex.net/skm/linux/skm
    - source_hash: sha256=31c4f5021cb2561050bf0d75005f70637f65613f5acf503a949f949c8a11d29a
    - name: /usr/bin/skm
    - mode: 0755
    - skip_verify: True

setup oneshot job:
  file.managed:
    - name: /lib/systemd/system/skm.service
    - source: salt://services/skm.service

skm.service:
  service.enabled
