extract terraform x64 linux archive:
  archive.extracted:
    - name: /tmp/terraform
    - source: https://releases.hashicorp.com/terraform/0.12.13/terraform_0.12.13_linux_amd64.zip
    - source_hash: sha256=63f765a3f83987b67b046a9c31acff1ec9ee618990d0eab4db34eca6c0d861ec
    - enforce_toplevel: False

move terraform to /usr/bin:
  file.managed:
    - name: /usr/bin/terraform
    - source: /tmp/terraform/terraform
    - mode: 755

cleanup extracted terraform archive:
  file.absent:
    - name: /tmp/terraform
