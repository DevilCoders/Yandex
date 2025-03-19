musl-dev:
  pkg.installed

/lib/libc.musl-x86_64.so.1:
  file.symlink:
    - target: /usr/lib/x86_64-linux-musl/libc.so
    - require:
      - pkg: musl-dev
