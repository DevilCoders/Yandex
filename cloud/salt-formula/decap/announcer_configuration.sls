modify_announcer_configuration_file:
    file.managed:
      - name: /etc/yadecap/anycast_announces.yaml
      - source: salt://{{ slspath }}/jinja_templates/rootfs/etc/yadecap/anycast_announces.yaml
      - template: jinja
      - makedirs: True


restart_yadecap_announcer:
    service.running:
      - name: yadecap-announcer
      - enable: True
      - watch:
          - file: modify_announcer_configuration_file