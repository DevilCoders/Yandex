# update sshd config according to dict below
{% set sshd_config = {
  'PermitRootLogin': 'no',
  'LogLevel': 'VERBOSE',
  'PubkeyAuthentication': 'yes',
  'ChallengeResponseAuthentication': 'no',
  'PasswordAuthentication': 'no',
  'PermitEmptyPasswords': 'no',
  'FingerprintHash': 'sha256'
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
