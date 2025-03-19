{% from slspath ~ "/map.jinja" import walg with context %}

walg-packages:
    pkg.installed:
        - pkgs:
            - wal-g-redis: {{walg.version}}
            - python-kazoo: 2.5.0-2yandex
            - python3-kazoo: 2.5.0
            - zk-flock
