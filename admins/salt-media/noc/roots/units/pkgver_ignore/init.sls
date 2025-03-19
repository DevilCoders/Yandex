/etc/yandex-pkgver-ignore.d/pkgver_ignore:
  file.managed:
    - source: salt://{{ slspath }}/pkgver_ignore
    - makedirs: True
