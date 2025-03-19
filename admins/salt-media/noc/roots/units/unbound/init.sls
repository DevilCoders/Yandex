yandex-unbound:
  pkg.installed

# local zones NOCDEV-7738
/etc/unbound/unbound.conf.d/70-local-zone.conf:
  file.managed:
    - source: salt://{{ slspath }}/70-local-zone.conf
