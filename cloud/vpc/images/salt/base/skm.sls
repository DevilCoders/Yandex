# Installs skm (https://wiki.yandex-team.ru/cloud/devel/platform-team/infra/skm/)
# and oneshot job which starts on every boot
# See https://bb.yandex-team.ru/projects/CLOUD/repos/paas-images/browse/paas-base-g2/salt/skm

install up-to-date SKM binary:
  file.managed:
    - source: https://storage.cloud-preprod.yandex.net/skm/linux/skm
    - source_hash: sha256=b4d4df33a3befbd489f727cb851917d10f35f8ef796ce88492ca047d6045a046
    - name: /usr/bin/skm
    - mode: 0755
    - skip_verify: True

install metadata wait script:
  file.managed:
    - name: /usr/bin/wait-metadata.sh
    - source: salt://{{ slspath }}/skm/wait-metadata.sh
    - mode: 0755

setup oneshot job:
  file.managed:
    - name: /lib/systemd/system/skm.service
    - source: salt://{{ slspath }}/skm/skm.service

skm.service:
  service.enabled
