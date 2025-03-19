{% from slspath ~ "/map.jinja" import walg with context %}

walg-packages:
    pkg.installed:
        - pkgs:
            - wal-g-mongo: {{walg.version}}

