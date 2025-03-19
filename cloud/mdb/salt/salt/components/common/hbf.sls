include:
    - components.monrun2.hbf-agent

hbf-agent:
    pkg.installed:
        - pkgs:
            - yandex-hbf-agent
            - yandex-hbf-agent-init
            - yandex-hbf-agent-monitoring
        - prereq_in:
            - cmd: repositories-ready

/etc/yandex-hbf-agent/yandex-hbf-agent.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/etc/yandex-hbf-agent/yandex-hbf-agent.conf
        - template: jinja
        - mode: 0644

yandex-hbf-agent:
    service.running:
        - restart: True
        - watch:
            - file: /etc/yandex-hbf-agent/yandex-hbf-agent.conf
        - require:
            - pkg: hbf-agent
