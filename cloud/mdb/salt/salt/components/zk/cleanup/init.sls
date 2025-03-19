zk-cleanup-packages:
  pkg.installed:
  - pkgs:
    - python3-click
    - python3-kazoo

/usr/local/yandex/zk_cleanup.py:
  file.managed:
  - source: salt://{{ slspath }}/zk_cleanup.py
  - mode: 755
  - makedirs: True
  - require:
      - pkg: zk-cleanup-packages
