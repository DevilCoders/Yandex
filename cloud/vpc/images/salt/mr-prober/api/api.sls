Authenticate in Docker Container Registry:
  cmd.run:
    - name: docker login --username iam --password $(curl -H Metadata-Flavor:Google http://169.254.169.254/computeMetadata/v1/instance/service-accounts/default/token | jq -r .access_token) cr.yandex

api:
  file.managed:
    - name: /etc/systemd/system/api.service
    - source: salt://{{ slspath }}/files/api.service
    - template: jinja
    - context:
        version: "{{ pillar["mr_prober"]["versions"]["api"] }}"
  module.run:
    - name: service.systemctl_reload
    - onchanges:
      - file: api
  service.enabled:
    - watch:
      - module: api
  docker_image.present:
    - name: cr.yandex/crpni6s1s1aujltb5vv7/api
    - tag: "{{ pillar["mr_prober"]["versions"]["api"] }}"
    - require:
      - cmd: Authenticate in Docker Container Registry

