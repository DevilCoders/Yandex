include:
  - common.juggler-client

{{ sls }}_packages:
  yc_pkg.installed:
    - pkgs:
      - yc-autorecovery
    - require:
      - file: /etc/yc/autorecovery/conf.d
      - file: /etc/juggler/client.conf 

/etc/yc/autorecovery/conf.d:
  file.directory:
    - user: root
    - group: root
    - mode: 755
    - makedirs: True
