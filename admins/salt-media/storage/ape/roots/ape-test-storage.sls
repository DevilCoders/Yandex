/etc/monrun/conf.d:
  file.recurse:
    - source: salt://ape-test-storage/etc/monrun/conf.d

/etc/elliptics:
  file.recurse:
    - source: salt://ape-test-storage/etc/elliptics

/etc/cron.d:
  file.recurse:
    - source: salt://ape-test-storage/etc/cron.d

/usr/local/bin:
  file.recurse:
    - source: salt://ape-test-storage/usr/local/bin

