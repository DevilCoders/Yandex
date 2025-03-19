redis-salt-module-requirements:
    pkg.installed:
        - pkgs:
            - python-redis: 2.10.6-2ubuntu1+yandex0
            - python36-redis: 3.5.3
            - python3-netifaces: 0.10.4-0.1build4
        - prereq_in:
            - cmd: repositories-ready

include:
    - components.repositories
