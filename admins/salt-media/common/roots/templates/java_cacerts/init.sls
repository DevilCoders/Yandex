yandex-internal-root-ca:
  pkg.installed

ca-certificates-java:
  pkg.installed:
    - pkg: yandex-internal-root-ca

/etc/yandex/cert/java-all.keystore:
  file.symlink:
    - target: /etc/ssl/certs/java/cacerts
    - mode: 644
    - makedirs: True
    - require:
      - pkg: ca-certificates-java
