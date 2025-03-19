/etc/cloud/templates/hosts.debian.tmpl:
  file.append:
    - text:
      - '169.254.169.254 metadata.google.internal'

# Remove the symlinked /etc/resolv.conf, otherwise it will be overwritten.
disable resolveconf:
  file.absent:
    - name: /etc/resolv.conf

# Enable Yandex DNS
/etc/resolv.conf:
  file.managed:
    - contents:
      - 'nameserver 2a02:6b8::1:1'
      - 'nameserver 2a02:6b8:0:3400::1'