Authenticate in Docker Container Registry:
  cmd.run:
    - name: docker login --username iam --password $(curl -H Metadata-Flavor:Google http://169.254.169.254/computeMetadata/v1/instance/service-accounts/default/token | jq -r .access_token) cr.yandex

creator:
  file.managed:
    - name: /etc/systemd/system/creator.service
    - source: salt://{{ slspath }}/files/creator.service
    - template: jinja
    - context:
        version: "{{ pillar["mr_prober"]["versions"]["creator"] }}"
  module.run:
    - name: service.systemctl_reload
    - onchanges:
      - file: creator
  service.enabled:
    - watch:
      - module: creator
  docker_image.present:
    - name: cr.yandex/crpni6s1s1aujltb5vv7/creator
    - tag: "{{ pillar["mr_prober"]["versions"]["creator"] }}"
    - require:
      - cmd: Authenticate in Docker Container Registry

