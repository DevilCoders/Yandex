# Source: https://a.yandex-team.ru/arc/trunk/arcadia/cloud/security/oslogin/salt/sshd/init.sls

include:
  - .google-compute-engine-oslogin
  - .breakglass

openssh-server:
  pkg.installed

/etc/ssh/yc_ca_keys.pub:
  file.managed:
    - user: root
    - group: root
    - mode: 0640
    - contents: >
        - ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQC/kScoku920oGZ8vGPy9aNV+aQtihyzyEZy40Gbr9c18fGFh9Nc8SdFba/e0JKH8eOJq7TLB4mcxM2GybdPVFzoW+SkNJZEgN++YHMED1PmHMPCDB9JmijJOIiOvnkOrKIIgn7/6g841qBCXL+hn+VySHPLSooAlVJnZXSdads6nv9kXZeyU4rHzOT7BkkK4Bcbay+04srmvShno87KB47hYH0Gb6nJH8VHY7RBuqhdyDzMQ2gq1n3wYF6aLgHphboP01LTsonjIVoBJhYT+Eyie95bsg5gt7XcQP02iBvmGh/7CtkG18QpABv8GLFF5R3YL/B4LG7AMyyOTyZc5Wz yubikey
        - ecdsa-sha2-nistp521 AAAAE2VjZHNhLXNoYTItbmlzdHA1MjEAAAAIbmlzdHA1MjEAAACFBAFxDHbZHZCx72sztvWsFSJVUhW+fngANgR5iiIWy9gicOdjYOy0pxpb1U8PqWmJYJYOWLwtfm/f9qnSV3Q/wAX/igCAwdYuwsWtx/0eBp2G3ECIUJzEQRme+TrtECpJHyLFCJonE/Cg24JlQ8N6mVjrQZwLiVjCdVwRFGoXxb3F1N4SHg== bastion
        - ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIF6dz6foYXUqqxfqJpI/ZHabt1OiMINrrLD/LKyZJ5N3 bastion
    - require:
      - pkg: openssh-server

sshd:
  file.managed:
    - name: /etc/ssh/sshd_config
    - source: salt://{{ slspath }}/sshd/sshd_config
    - user: root
    - group: root
    - mode: 0600
    - template: jinja
    - defaults:
      trusted_user_ca_keys: /etc/ssh/yc_ca_keys.pub
    - require:
      - pkg: openssh-server
      - file: /etc/ssh/yc_ca_keys.pub

  service.running:
    - enable: True
    - watch:
      - pkg: openssh-server
      - file: sshd

