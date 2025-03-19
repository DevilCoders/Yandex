/etc/sudoers:
    file.managed:
        - source: salt://components/common/conf/etc/sudoers
        - mode: 440
        - user: root
        - group: root

/etc/sysctl.conf:
    file.managed:
        - template: jinja
        - source: salt://components/common/conf/etc/sysctl.conf
        - mode: 644
        - user: root
        - group: root
    cmd.wait:
        - name: 'sysctl -p /etc/sysctl.conf && if ls /etc/sysctl.d/*.conf 2>/dev/null; then sysctl -p /etc/sysctl.d/*.conf; fi'
        - watch:
            - file: /etc/sysctl.conf

/etc/ssh/ssh_config:
    file.managed:
        - source: salt://components/common/conf/etc/ssh/ssh_config
        - mode: 644
        - user: root

/etc/ssh/sshd_config:
    file.managed:
        - source: salt://components/common/conf/etc/ssh/sshd_config
        - mode: 644
        - user: root
        - group: root
        - template: jinja

sshd-service:
    service.running:
        - enable: True
        - name: ssh
        - reload: True
        - require:
            - service: ssh.socket-disable
        - watch:
            - file: /etc/ssh/sshd_config

ssh.socket-disable:
  service.dead:
    - name: ssh.socket
    - enable: False
    - onlyif:
        - test -f /etc/systemd/system/sockets.target.wants/ssh.socket

root:
    user.present:
        - password: '$6$AmLQ4fU3$/0kqz7HlZRatHL6wCvgoqTed6JdHdNAStxKF2x0fCgRSQGqnIWvpPtazGN.RtgAZpdWvPBUnt/k2aBrUyGInW/'

/var/cores:
    file.directory

/etc/cron.yandex:
    file.directory

/etc/profile.d/colorize.sh:
    file.managed:
        - source: salt://components/common/conf/etc/profile.d/colorize-bionic.sh
        - mode: 644

/root/.bashrc:
    file.managed:
        - name: /root/.bashrc
        - source: salt://{{ slspath }}/conf/root_bashrc
        - template: jinja

ondemand:
    service.disabled

/usr/local/yandex:
    file.directory:
        - user: root
        - group: root
        - mode: 0755

/etc/cron.d/apt-cache-cleanup:
    file.managed:
        - source: salt://{{ slspath }}/conf/etc/cron.d/apt-cache-cleanup
        - mode: 644
