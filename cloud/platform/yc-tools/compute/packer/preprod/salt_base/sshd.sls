# update sshd config according to dict below
{% set sshd_config = {
  'PermitRootLogin': 'no',
  'LogLevel': 'VERBOSE',
  'PubkeyAuthentication': 'yes',
  'ChallengeResponseAuthentication': 'no',
  'PasswordAuthentication': 'no',
  'PermitEmptyPasswords': 'no',
  'FingerprintHash': 'sha256',
  'TrustedUserCAKeys': '/etc/ssh/ca.pub'
  }
%}

{% for key, value in sshd_config.iteritems() %}
sshd_config_{{ key }}_{{ value }}:
  file.replace:
    - name: /etc/ssh/sshd_config
    - pattern: '^{{ key }} .*'
    - repl: '{{ key }} {{ value }}'
    - append_if_not_found: True
{% endfor %}


# provide rsyslog config for trapdoor
/etc/rsyslog.d/99-ssh-trapdoor.conf:
  file.managed:
    - contents: 'auth,authpriv.* @trapdoor.yandex.net'

# CLOUD-32787
CA public key:
  file.managed:
    - name: /etc/ssh/ca.pub
    - contents: |
        ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQC/kScoku920oGZ8vGPy9aNV+aQtihyzyEZy40Gbr9c18fGFh9Nc8SdFba/e0JKH8eOJq7TLB4mcxM2GybdPVFzoW+SkNJZEgN++YHMED1PmHMPCDB9JmijJOIiOvnkOrKIIgn7/6g841qBCXL+hn+VySHPLSooAlVJnZXSdads6nv9kXZeyU4rHzOT7BkkK4Bcbay+04srmvShno87KB47hYH0Gb6nJH8VHY7RBuqhdyDzMQ2gq1n3wYF6aLgHphboP01LTsonjIVoBJhYT+Eyie95bsg5gt7XcQP02iBvmGh/7CtkG18QpABv8GLFF5R3YL/B4LG7AMyyOTyZc5Wz crt.cloud.yandex.net
