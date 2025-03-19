selfdns packages:
  pkg.installed:
    - pkgs:
      - yandex-selfdns-client
      - yc-selfdns-plugins

/lib/systemd/system/selfdns-client.service:
  file.managed:
    - source: salt://services/selfdns-client.service

selfdns_client_service:
  service.enabled:
    - name: selfdns-client.service