grpcurl archive:
  archive.extracted:
    - name: /tmp/grpcurl
    - source: https://github.com/fullstorydev/grpcurl/releases/download/v1.8.0/grpcurl_1.8.0_linux_x86_64.tar.gz
    - source_hash: sha256=7261c1542cf139b0663b10948fe53578322cc36e5322406b4e231564f91712f1
    - enforce_toplevel: False
    - list_options: grpcurl

move grpcurl to /usr/bin:
  file.managed:
    - name: /usr/bin/grpcurl
    - source: /tmp/grpcurl/grpcurl
    - mode: 755

cleanup extracted archive:
  file.absent:
    - name: /tmp/grpcurl