# https://github.yandex-team.ru/mds-storage/yandex-storage-mds-autoadmin/blob/master/mds_autoadmin/shelves_check.py
shelves-check:
  pkg.installed:
    - pkgs:
      - yandex-storage-mds-autoadmin
  monrun.present:
    - command: "/usr/bin/shelves_check"
    - execution_interval: 300
    - execution_timeout: 180
    - type: storage

/etc/monitoring/shelves-check-fwlist:
  file.managed:
    - contents: |
        {"ST10000NM0016-1TT101":"SNYD","ST8000NM0055-1RM112":"SN04","ST8000NM0016-1U3101":"SNYD", "ST12000NM0007-2A1101": "SN02", "ST6000NM0024-1HT17Z": ["SN05", "SN06"]}
    - makedirs: True
