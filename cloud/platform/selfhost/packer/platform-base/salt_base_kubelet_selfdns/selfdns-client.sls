selfdns packages:
  pkg.installed:
    - pkgs:
      - yandex-selfdns-client
      - yc-selfdns-plugins

/lib/systemd/system/selfdns-client.service:
  file.managed:
    - source: salt://services/selfdns-client.service

/usr/bin/selfdns-client-token-getter.sh:
  file.managed:
    - source: salt://files/selfdns-client-token-getter.sh
    - mode: '0755'

selfdns_client_service:
  service.enabled:
    - name: selfdns-client.service