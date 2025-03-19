install_bird:
    yc_pkg.installed:
      - pkgs:
          - bird


modify_configuration_files:
    file.recurse:
      - name: /etc/bird
      - source: salt://{{ slspath }}/jinja_templates/rootfs/etc/bird
      - template: jinja
      - makedirs: True
      - user: bird
      - group: bird
      - require: 
          - yc_pkg: install_bird


add_blank_file:
    file.managed:
      - name: /etc/bird/bird.conf.static_routes
      - source: salt://{{ slspath }}/plain_files/rootfs/etc/bird/bird.conf.static_routes
      - makedirs: True
      - user: bird
      - group: bird
      - require: 
          - yc_pkg: install_bird


create_bird_log_file:
    file.managed:
      - name: /var/log/bird.log
      - makedirs: True
      - user: bird
      - group: bird
      - watch:
          - yc_pkg: install_bird


enable_bird_service:
    service.running:
      - name: bird
      - enable:  True
      - restart: True
      - watch:
          - yc_pkg: install_bird
          - file: modify_configuration_files
          - file: add_blank_file
          - file: create_bird_log_file


delete_empty_config_file:
    file.absent:
      - name: /etc/bird/bird.conf.static_routes
      - require: 
          - service: enable_bird_service