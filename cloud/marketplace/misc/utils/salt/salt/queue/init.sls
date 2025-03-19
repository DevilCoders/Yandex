{% set config_path='/etc/yc-marketplace/' + sls + '-config.yaml' %}

{{ config_path }}:
  file.managed:
    - source: salt://queue/files/config.yaml
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - template: jinja

cr_static_key:
  file.managed:
  - name: /etc/yc-marketplace/cr_static_key.json
  - user: root
  - group: root
  - mode: 0600
  - contents_pillar: cr_static_key

login to docker cr.yandex:
  cmd.run:
    - name: cat /etc/yc-marketplace/cr_static_key.json | sudo docker login --username json_key --password-stdin cr.yandex && sudo service docker start


'Queue Service account key':
  file.managed:
  - name: /etc/yc-marketplace/service_key
  - user: root
  - group: root
  - mode: 0600
  - contents_pillar: service_account:key


queue_run:
  docker_container.running:
    - image: {{ pillar['common']['docker_queue'] }}
    - name: marketplace-worker
    - log_driver: journald # TODO syslog is better
    - environment:
      - LOGLEVEL: {{ pillar['common']['loglevel'] }}
      - MARKETPLACE_CLOUD_ID: {{ pillar['api']['cloud_id'] }}
      - REQUESTS_CA_BUNDLE: /etc/ssl/certs/ca-certificates.crt
      - PICTURE_S3_ACCESS_KEY: {{ pillar['security']['api']['s3']['access_key'] }}
      - PICTURE_S3_SECRET_KEY: {{ pillar['security']['api']['s3']['secret_key'] }}
      - YDB_CLIENT_VERSION: 2
      - YC_CONFIG_PATH: {{ config_path }}
      - LOGBROKER_TVM_CLIENT_ID: {{ pillar['security']['log_reader_tvm']['client_id'] }}
      - LOGBROKER_TVM_SECRET: {{ pillar['security']['log_reader_tvm']['secret'] }}
      - MARKETPLACE_SENDMAIL_LOGIN: robot-marketplace
      - MARKETPLACE_SENDMAIL_PASS: {{ pillar['security']['yndx_cloud_mkt_pass'] }}
    - network_mode: host
    - restart_policy: unless-stopped
    - binds:
      - /etc/yc-marketplace/:/etc/yc-marketplace/:ro
      - /run/systemd/journal/:/run/systemd/journal/:rw

  cmd.run:
    - name: docker restart marketplace
    - onchanges_any:
      - file: {{ config_path }}


{%- from slspath+"/monitoring.yaml" import monitoring -%}
{%- include "common/deploy_mon_scripts.sls" %}
