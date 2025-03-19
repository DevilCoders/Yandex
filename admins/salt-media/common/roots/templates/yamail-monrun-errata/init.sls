yamail-monrun-errata:
  pkg:
    - installed

/usr/local/bin/update-errata:
  yafile.managed:
    - source: salt://templates/yamail-monrun-errata/files/usr/local/bin/update-errata
    - mode: 755
    - user: root
    - group: root
    - makedirs: True