include:
  - google-compute-engine-oslogin

libpam-modules: pkg.installed

/etc/pam.d/sshd:
  file.managed:
    - source: salt://{{ slspath }}/configs/sshd
    - user: root
    - group: root
    - mode: 0644
    - require:
      - sls: google-compute-engine-oslogin
      - pkg: libpam-modules

/etc/pam.d/su:
  file.managed:
    - source: salt://{{ slspath }}/configs/su
    - user: root
    - group: root
    - mode: 0644
    - require:
      - sls: google-compute-engine-oslogin
      - pkg: libpam-modules

/var/google-users.d:
  file.directory:
    - user: root
    - group: root
    - mode: 0750
    - require:
      - file: /etc/pam.d/sshd
