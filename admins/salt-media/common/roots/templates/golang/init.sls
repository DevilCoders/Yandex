golang:
  file.managed:
    - name: /tmp/golang.tar.gz
    - source: {{ salt['pillar.get']('golang:url', 'https://dl.google.com/go/go1.14.linux-amd64.tar.gz') }}
    - source_hash: sha256=08df79b46b0adf498ea9f320a0f23d6ec59e9003660b4c9c1ce8e5e2c6f823ca
  cmd.run:
    - name: tar -C /usr/local -xzf /tmp/golang.tar.gz
    - unless: test -e /usr/local/go/bin/go

