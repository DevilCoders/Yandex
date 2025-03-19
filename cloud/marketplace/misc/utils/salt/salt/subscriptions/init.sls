{% set config_path='/etc/yc-marketplace/' + sls + '/config.yaml' %}
{% set image='registry.yandex.net/marketplace/cloud-api/mrkt-subscriptions' %}

{{ config_path }}:
  file.managed:
    - source: salt://subscriptions/files/config.yaml
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - template: jinja

'Subscriptions Service account key':
  file.managed:
  - name: /etc/yc-subscriptions/service_key
  - user: root
  - group: root
  - mode: 0600
  - contents_pillar: service_account:key

{% set subscriptions_api=image + ':' + pillar['common']['subscriptions_version'] %}

subscriptions:
  docker_container.running:
    - image: {{ subscriptions_api }}
    - name: mrkt-subscriptions
    - log_driver: journald # TODO syslog is better
    - environment:
      - MARKETPLACE_PENDING_IMG_FOLDER_ID: {{ pillar['api']['pending_img_folder_id'] }}
      - MARKETPLACE_PUBLIC_IMG_FOLDER_ID: {{ pillar['api']['public_img_folder_id'] }}
      - MARKETPLACE_OAUTH_TOKEN: {{ pillar['security']['marketplace_token'] }}
      - MARKETPLACE_CLOUD_ID: {{ pillar['api']['cloud_id'] }}
      - PICTURE_S3_ACCESS_KEY: {{ pillar['security']['api']['s3']['access_key'] }}
      - PICTURE_S3_SECRET_KEY: {{ pillar['security']['api']['s3']['secret_key'] }}
      - KIKIMR_CLIENT_EXPERIMENT: OLD
    - network_mode: host
    - restart_policy: unless-stopped
    - binds:
      - /etc/yc-marketplace/:/etc/yc-marketplace/:ro
      - /run/systemd/journal/:/run/systemd/journal/:rw

{%- from slspath+"/monitoring.yaml" import monitoring -%}
{%- include "common/deploy_mon_scripts.sls" %}
