/opt/ycloud-tools/kubernetes-creds-daemon:
  file.managed:
    {# source code: https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/container-registry/kubernetes-creds-daemon #}
    - source: https://storage.yandexcloud.net/ycr-tools/ycr-tools/v8-82cdd3ac1/linux/amd64/kubernetes-creds-daemon
    - source_hash: be4d5f61314970e2238d6b12655a1441
    - user: root
    - makedirs: True
    - mode: 755

Set registries list depending on environment:
  cmd.script:
    - name: salt://scripts/resolve-registries.sh
    - args: /opt/ycloud-tools/registries-hostnames
    - runas: root


/etc/systemd/system/kubernetes-creds-daemon.service:
  file.managed:
    - source: salt://services/kubernetes-creds-daemon.service
    - user: root

kubernetes-creds-daemon.service:
  service.enabled
