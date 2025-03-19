{% from slspath ~ "/map.jinja" import sentinel with context %}

sentinel-pkgs:
    pkg.installed:
        - pkgs:
            - redis-sentinel: {{ sentinel.version.pkg }}
            - redis-tools: {{ sentinel.version.pkg }}
        - require:
            - file: sentinel-config
            - cmd: repositories-ready
