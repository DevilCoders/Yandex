/etc/monrun/salt_redis-status/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True
  pkg.installed:
    - pkgs:
      - yandex-media-common-oom-check

/etc/monrun/salt_redis-status/redis_status.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/redis_status.sh
    - makedirs: True
    - mode: 755
