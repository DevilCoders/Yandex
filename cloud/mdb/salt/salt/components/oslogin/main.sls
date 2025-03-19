metadata.google.internal-to-169.254.169.254:
  host.present:
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
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - contents:
      - breakglass

{# SSHd #}
/var/google-users.d:
  file.directory:
    - user: root
    - group: root
    - mode: 0750
    - require:
      - test: oslogin-pkgs-done

/etc/ssh/authorized_principals:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - contents_pillar: data:oslogin:breakglass:authorized_principals

/etc/ssh/yc_ca_keys.pub:
  file.managed:
    - user: root
    - group: root
    - mode: 0640
    - contents_pillar: data:oslogin:breakglass:ca_public_keys

{# sudo #}
/var/google-sudoers.d:
  file.directory:
    - user: root               
    - group: root              
    - mode: 0750               
    - require:
      - test: oslogin-pkgs-done
  
/etc/sudoers.d/google-oslogin: 
  file.managed:
    - user: root               
    - group: root
    - mode: 0440               
    - contents:                
      - '#includedir /var/google-sudoers.d'

{# sudoers for L2 support #}
/etc/sudoers.d/l2_support:
  file.managed:
    - source: salt://{{ slspath }}/conf/l2_sudoers
    - user: root               
    - group: root
    - mode: 0600               
    - template: 'jinja'
