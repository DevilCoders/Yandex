grpcurl archive:
  archive.extracted:
    - name: /tmp/grpcurl
    - source: https://github.com/fullstorydev/grpcurl/releases/download/v1.3.1/grpcurl_1.3.1_linux_x86_64.tar.gz
    - source_hash: sha1=12346f44c646d87fc96de4b3c7ef5b5824d0a231
    - enforce_toplevel: False
    - list_options: grpcurl

move grpcurl to /usr/bin:
  file.managed:
    - name: /usr/bin/grpcurl
    - source: /tmp/grpcurl/grpcurl

cleanup extracted archive:
  file.absent:
    - name: /tmp/grpcurl

