config:
  yafile.managed:
    - name: /etc/yandex/pingunoque/config.yaml
    - makedirs: true
    - source: salt://common/files/etc/yandex/pingunoque/config.yaml

pkg:
  pkg.installed:
    - name: pingunoque
  service.running:
    - name: pingunoque
    - enable: true
    - reload: true
