/usr/lib/yandex/node/svgo/package.json:
  file.managed:
    - source: salt://{{ slspath }}/package.json
    - makedirs: True

install_svgo:
  cmd.run:
    - name: /opt/nodejs/4/bin/npm install --prefix /usr/lib/yandex/node/svgo
    - watch:
        - file: /usr/lib/yandex/node/svgo/package.json
