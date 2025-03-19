yandex-media-java-healthcheck-divider:
  pkg.installed

/etc/monrun/conf.d/custom_api_check.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - source: salt://{{ slspath }}/files/custom_api_check.conf

/etc/config-monrun-cpu-check/config.yml:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - contents: |
        warning: 101
        critical: 101

regen_monrun:
  cmd.run:
    - name: /usr/sbin/regenerate-monrun-tasks
    - watch:
      - file: /etc/monrun/conf.d/custom_api_check.conf
