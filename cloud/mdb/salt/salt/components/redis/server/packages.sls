{% from slspath ~ "/map.jinja" import redis with context %}

redis-pkgs:
    pkg.installed:
        - pkgs:
            - redis-tools: {{ redis.version.pkg }}
            - redis-server: {{ redis.version.pkg }}
        - require:
            - file: redis-main-config
            - cmd: repositories-ready
