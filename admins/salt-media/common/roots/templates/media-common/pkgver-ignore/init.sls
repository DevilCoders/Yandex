/etc/yandex-pkgver-ignore.d:
  file.recurse:
    - source: salt://{{ slspath }}/yandex-pkgver-ignore.d
    - include_empty: True
