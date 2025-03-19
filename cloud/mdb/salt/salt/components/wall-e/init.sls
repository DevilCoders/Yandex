wall-e-client-pkgs:
    pkg.installed:
        - pkgs:
            - wall-e-checks-bundle: 1.29-9222073
            - yandex-wall-e-agent: 2.0.2-1
{% if salt['pillar.get']('data:wall-e:repair', False) %}
            - yandex-search-hw-watcher-walle-config: '1.10'
{% endif %}
        - prereq_in:
            - cmd: repositories-ready
        - watch_in:
            - cmd: juggler-client-restart

{% if salt['pillar.get']('data:wall-e:repair', False) %}
/etc/hw_watcher/conf.d/walle_custom.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/walle_custom.conf' }}
        - require:
            - pkg: wall-e-client-pkgs
{% endif %}
