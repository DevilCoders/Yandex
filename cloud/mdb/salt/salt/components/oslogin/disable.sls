purge-oslogin-packages:
  pkg.purged:
    - name: google-compute-engine-oslogin

drop-metadata.google.internal-to-169.254.169.254:
  host.absent:
    - ip: 169.254.169.254
    - names:
      - metadata.google.internal

{# NSSwitch #}
/etc/nsswitch.conf:
  file.managed:
    - source: salt://{{ slspath }}/conf/nsswitch.conf
    - user: root
    - group: root
    - mode: 0644
    - template: jinja

{# PAM #}
/etc/pam.d/sshd:
  file.managed:
    - source: salt://{{ slspath }}/conf/pam.d/sshd
    - user: root
    - group: root
    - mode: 0644
    - template: jinja

/etc/pam.d/su:
  file.managed:
    - source: salt://{{ slspath }}/conf/pam.d/su
    - user: root
    - group: root
    - mode: 0644
    - template: jinja

/etc/security/breakglass_users:
  file.absent

{# SSHd #}
/var/google-users.d:
  file.absent

/etc/ssh/authorized_principals:
  file.absent

/etc/ssh/yc_ca_keys.pub:
  file.absent

{# sudo #}
/var/google-sudoers.d:
  file.absent
  
/etc/sudoers.d/google-oslogin: 
  file.absent

{# breakglass user #}
breakglass:
  user.absent

/etc/sudoers.d/breakglass:
  file.absent

/etc/sudoers.d/l2_support:
  file.absent
