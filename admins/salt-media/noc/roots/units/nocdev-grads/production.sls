/etc/wireguard/wg0.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/wireguard/wg0.conf
    - template: jinja
    - user: root
    - group: root
    - mode: 640
    - makedirs: True

wireguard:
  pkg.installed: []
  service.running:
    - enable: True
    - name: wg-quick@wg0
    - watch:
        - file: /etc/wireguard/wg0.conf

/etc/grad/monitor_nocauth_rsa:
  file.managed:
    - contents: {{ pillar['sec']['monitor_nocauth_rsa'] | json }}
    - user: root
    - group: root
    - mode: 440

/etc/grad/conf.d:
  file.recurse:
    - source: salt://{{ slspath }}/files/etc/grad/conf.d
    - template: jinja
    - user: root
    - group: root
    - file_mode: 440
    - dir_mode: 755
    - makedirs: True
    - clean: True

/etc/grad/credentials.yml:
  file.managed:
  - source: salt://{{ slspath }}/files/etc/grad/credentials.yml
  - template: jinja
  - user: grad
  - group: grad
  - mode: 600
  - makedirs: True

/etc/grad/grad_series.yml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/grad/grad_series.yml
    - template: jinja
    - user: grad
    - group: grad
    - mode: 600
    - makedirs: True

# NOCDEV-7136
/etc/openvpn/sdc_client.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/openvpn/sdc_client.conf
    - user: root
    - group: root
    - mode: 640
    - makedirs: True

/etc/openvpn/sdc_ca.crt:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/openvpn/sdc_ca.crt
    - user: root
    - group: root
    - mode: 400

/etc/openvpn/sdc_cert.crt:
  file.managed:
    - contents: {{ pillar['sdc_sec'][grains['fqdn'] + '_cert'] | json }}
    - user: root
    - group: root
    - mode: 400

/etc/openvpn/sdc_key.key:
  file.managed:
    - contents: {{ pillar['sdc_sec'][grains['fqdn'] + '_key'] | json }}
    - user: root
    - group: root
    - mode: 400

openvpn:
  pkg.installed: []
  service.running:
    - enable: True
    - name: openvpn@sdc_client
    - watch:
      - file: /etc/openvpn/sdc_client.conf

/etc/systemd/system/openvpn@.service.d/override.conf:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files/etc/systemd/system/openvpn@.service.d/override.conf
