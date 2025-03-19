{% set osrelease = salt.grains.get('osrelease') %}

mdb-mongo-tools-packages:
    pkg.installed:
        - pkgs:
            - mdb-mongo-tools: 2.26-4d18ffb
            - libpython3.6
            - libpython3.6-minimal
            - libpython3.6-stdlib
            - python3.6
            - python3.6-minimal
            - python3.6-venv
            - python3-yaml: 3.12-1build2
            - zk-flock
        - require:
            - pkgrepo: mdb-bionic-stable-all
