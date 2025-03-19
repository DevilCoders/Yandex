nbs_client_packages:
  yc_pkg.installed:
    - pkgs:
      - yandex-cloud-blockstore-plugin
      - yandex-cloud-blockstore-client
    - disable_update: True
    - require:
      - file: /etc/yc/nbs/client/client.txt

/etc/yc/nbs/client/client.txt:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/files/client/client.txt
    - template: jinja
