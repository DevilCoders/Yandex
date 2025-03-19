{% set osrelease = salt['grains.get']('osrelease') %}
include:
    - components.hw-watcher
    - components.monrun2.hbf-agent
    - components.monrun2.dom0porto
    - components.linux-kernel
    - .common
    - .images
    - .containers
    - .mdb-metrics
    - .mdb-porto-agent
    - components.yasmagent
    - .yasmagent
