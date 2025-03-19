/usr/lib/yandex-graphite-checks/enabled/hbf.sh:
  file.managed:
    - user: root
    - group: root
    - mode: 0755
    - makedirs: True
    - contents: |
        #!/usr/bin/env bash
        /usr/bin/hbf-monitoring-drops-graphite -o simple
