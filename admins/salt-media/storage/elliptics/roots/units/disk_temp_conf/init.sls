/etc/monitoring/disk_temp.conf:
  file.managed:
    - contents: |
        temp_threshold=58
        max_warm_disks=1
    - user: root
    - group: root
    - mode: 644
