sshd:
  service.running:
    - reload: True
    - watch:
      - file: /etc/ssh/sshd_config

# REMOVEME later
/etc/ssh/sshd_config:
  file.line:
    - mode: delete
    - content: "AcceptEnv JAIL_HOSTNAME"

# REMOVEME later
/etc/sudoers.d/yc-bastion: file.absent

# REMOVEME later
/etc/profile.d/non-bastion-warning.sh: file.absent

/root/.ssh/authorized_keys2:
  file.managed:
    - source: salt://{{ slspath }}/files/authorized_keys2
    - user: root
    - group: root
    - mode: 600
