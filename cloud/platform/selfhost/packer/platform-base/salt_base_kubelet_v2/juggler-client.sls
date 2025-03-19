/juggler-bundle:
  file.directory:
    - user: root
    - group: root

extract_juggler-bundle:
  archive.extracted:
    - name: /juggler-bundle/
    - source: https://proxy.sandbox.yandex-team.ru/1112386298
    - skip_verify: True
    - archive_format: tar
    - enforce_toplevel: False

/etc/juggler/client.conf:
  file.managed:
    - source: salt://configs/juggler-client/client.conf

restart-juggler-client:
  service.running:
    - name: juggler-client
    - listen:
      - file: /etc/juggler/client.conf
