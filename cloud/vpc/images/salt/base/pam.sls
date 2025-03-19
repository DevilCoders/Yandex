include:
  - .google-compute-engine-oslogin

libpam-modules:
  pkg.installed

/etc/security/breakglass_users:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - contents: breakglass

/etc/pam.d/sshd:
  file.managed:
    - source: salt://{{ slspath }}/pam/sshd
    - user: root
    - group: root
    - mode: 0644
    - template: jinja
    - defaults:
        breakglass_users_file: /etc/security/breakglass_users
    - require:
      - file: /etc/security/breakglass_users
      - pkg: google-compute-engine-oslogin
      - pkg: libpam-modules

/etc/pam.d/su:
  file.managed:
    - source: salt://{{ slspath }}/pam/su
    - user: root
    - group: root
    - mode: 0644
    - require:
      - pkg: google-compute-engine-oslogin
      - pkg: libpam-modules

/var/google-users.d:
  file.directory:
    - user: root
    - group: root
    - mode: 0750
    - require:
      - file: /etc/pam.d/sshd
