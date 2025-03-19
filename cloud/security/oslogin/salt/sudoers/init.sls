include:
  - google-compute-engine-oslogin

{% set sudoers_dir = '/var/google-sudoers.d' %}

sudo: pkg.installed

{{ sudoers_dir }}:
  file.directory:
    - user: root
    - group: root
    - mode: 0750
    - require:
      - sls: google-compute-engine-oslogin

/etc/sudoers.d/google-oslogin:
  file.managed:
    - user: root
    - group: root
    - mode: 0440
    - contents:
      - '#includedir {{ sudoers_dir }}'
    - require:
      - pkg: sudo
      - file: {{ sudoers_dir }}
