/usr/local/share/ca-certificates/:
  file.directory:
    - makedirs: True

download_ya_int_ca:
  cmd.run:
    - name: wget https://crls.yandex.net/YandexInternalRootCA.crt -O /usr/local/share/ca-certificates/YandexInternalRootCA.crt
    - require:
      - file: /usr/local/share/ca-certificates/

run_update_ca_certs:
  cmd.run:
    - name: update-ca-certificates
    - require:
      - cmd: download_ya_int_ca
