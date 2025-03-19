/usr/bin/lsiutil:
  yafile.managed:
    - source: salt://files/storage/lsiutil
    - mode: 755
    - user: root
    - group: root
    - makedirs: True

/usr/bin/xflash:
  file.managed:
    - source: salt://files/storage/xflash
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
