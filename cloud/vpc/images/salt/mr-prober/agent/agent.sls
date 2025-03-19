Authenticate in Docker Container Registry:
  cmd.run:
    - name: docker login --username iam --password $(curl -H Metadata-Flavor:Google http://169.254.169.254/computeMetadata/v1/instance/service-accounts/default/token | jq -r .access_token) cr.yandex

agent:
  file.managed:
    - name: /etc/systemd/system/agent.service
    - source: salt://{{ slspath }}/files/agent.service
    - template: jinja
    - context:
        version: "{{ pillar["mr_prober"]["versions"]["agent"] }}"
  module.run:
    - name: service.systemctl_reload
    - onchanges:
      - file: agent
  service.enabled:
    - watch:
      - module: agent
  docker_image.present:
    - name: cr.yandex/crpni6s1s1aujltb5vv7/agent
    - tag: "{{ pillar["mr_prober"]["versions"]["agent"] }}"
    - require:
      - cmd: Authenticate in Docker Container Registry

agent-logs:
  pkg.installed:
    - name: logrotate
  file.managed:
    - name: /etc/logrotate.d/mr_prober_agent
    - source: salt://{{slspath}}/files/agent.logrotate


/var/log/mr_prober/probers:
  file.directory:
    - makedirs: True


yc-mr-prober-agent:
  file.managed:
    - name: /usr/bin/yc-mr-prober-agent
    - source: salt://{{slspath}}/files/yc-mr-prober-agent
    - mode: 755


set static interfaces file:
  file.managed:
    - name: /etc/netplan/10-netcfg.yaml
    - source: salt://{{ slspath }}/files/netcfg.yaml


# Osqueryd consumes too many CPU for small agent VMs
disable osqueryd:
  service.dead:
    - name: osqueryd
    - enable: False


update_bashrc_root:
  file.managed:
    - name: '/root/.bashrc'
    - source: salt://{{ slspath }}/files/bashrc_skel.sh
    - user: root
    - group: root
    - mode: 0644

update_bashrc_skel:
  file.managed:
    - name: '/etc/skel/.bashrc'
    - source: salt://{{ slspath }}/files/bashrc_skel.sh
    - user: root
    - group: root
    - mode: 0644

limit_journald_files:
  file.keyvalue:
    - name: '/etc/systemd/journald.conf'
    - key: 'SystemMaxUse'
    - value: '300M'
    - separator: '='
    - uncomment: '# '
    - append_if_not_found: True
