supervisor-packages:
    pkg.installed:
        - pkgs:
            - supervisor: 3.3.1-1yandex

/etc/supervisor/supervisord.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/supervisord.conf

supervisor-service:
    service.running:
        - enable: true
        - name: supervisor
        - watch:
            - pkg: supervisor-packages
            - file: /etc/supervisor/supervisord.conf
