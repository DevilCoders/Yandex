include:
  - .global

/etc/apt/sources.list.d/rsyslog-ppa.list:
  file.managed:
    - order: 0
    - makedirs: True
    - contents: deb [arch=amd64] https://mirror.yandex.ru/mirrors/launchpad/adiscon/v8-stable/ {{grains["oscodename"]}} main 
/etc/apt/trusted.gpg.d/rsyslog-ppa.gpg:
  file.decode:
    - order: 0
    - encoding_type: base64
    - encoded_data: |
        xo0EUwItYAEEALrIMClrWcovMonWFWTjMpXnNmowTxHT/e2IhZ6J40mKhGWa2Y4iqfPVZgVwZFaQ
        7BuNKjV8lIByFGiI11V+ASNFvxOwVR/iX6RsQ4k3iCp+dOlcuO7u6XNeb4otkpfyYgmUlMfsyj+r
        QwnzoInPgjQsFf6M/+aMetNG+SIidX/HABEBAAHNGUxhdW5jaHBhZCBQUEEgZm9yIEFkaXNjb27C
        uAQTAQIAIgUCUwItYAIbAwYLCQgHAwIGFQgCCQoLBBYCAwECHgECF4AACgkQD23YE1I0vysxrgP/
        Y+oWmi3uY/KqfHdsD/cH/BicOFoAAhDembljG/UaAk4XUEv8sHzuZO12U4hyR4btrEnUfaRNDf+v
        LVkZwOWupcWVfJiLduvZSbpFydgHQgaA0cdk7FZSreIY63BumoZzVhMkPOtyX5joVHQPM/+xvpqA
        JKhuj+bSTGullYduGsQ=
    - require_in:
      - file: /etc/apt/sources.list.d/rsyslog-ppa.list
      - pkg: rsyslog packages
rsyslog packages:
  pkg.installed:
    - pkgs:
      - rsyslog: '>=8.2204'
      - rsyslog-relp
      - rsyslog-imptcp
      - rsyslog-mmjsonparse
      - rsyslog-mmrm1stspace
      - rsyslog-omstdout

rsyslog:
  cmd.run:
    - name: rsyslogd -N1 -f /etc/rsyslog.conf
    - onchanges:
      - file: /etc/rsyslog.*
  service.running:
    - enable: True
    - watch:
      - file: /etc/rsyslog.*
    - require:
      - cmd: rsyslog

/var/log/rsyslog/:
  file.directory:
    - user: syslog
    - group: syslog
    - dir_mode: 0755

/etc/rsyslog.d/20-ufw.conf:
  file.absent
