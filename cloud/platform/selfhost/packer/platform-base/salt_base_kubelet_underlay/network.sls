remove all network config:
  file.directory:
    - names:
      - /etc/network/interfaces.d
      - /etc/network/if-pre-up.d
      - /etc/network/if-down.d
      - /etc/network/if-post-down.d
      - /etc/network/if-up.d
    - clean: True
    - exclude_pat: '*_yc_*'

/etc/cloud/templates/hosts.debian.tmpl:
  file.append:
    - text:
      - '169.254.169.254 metadata.google.internal'

# Fix the issue that localhost doesn't resolve into ::1
update-hosts:
  file.replace:
    - name: /etc/cloud/templates/hosts.debian.tmpl
    - pattern: '(^.*ip6-loopback).*'
    - repl: '\1 localhost'

yc-network-config:
  cmd.run:
    - name: "apt-get -o DPkg::options::=--force-confmiss -o DPkg::options::=--force-confnew install --allow-downgrades -y yc-network-config=0.1-1.190514"
    - shell: /bin/bash
    - env:
      - LC_ALL: C.UTF-8
      - LANG: C.UTF-8

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

# This is a horrible hack for getting underlay IP address without Cloud Support.
/usr/bin/print-ip-to-ttyS0.sh:
  file.managed:
    - mode: 755
    - source: salt://files/print-ip-to-ttyS0.sh

/lib/systemd/system/print-ip-to-ttyS0.service:
  file.managed:
    - source: salt://services/print-ip-to-ttyS0.service

print_ip_to_ttyS0_service:
  service.enabled:
    - name: print-ip-to-ttyS0.service