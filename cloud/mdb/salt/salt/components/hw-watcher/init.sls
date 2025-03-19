{% if salt['grains.get']('virtual', 'physical') == 'physical' %}
hw-watcher-pkg:
    pkg.installed:
        - pkgs:
            - yandex-hw-watcher: '0.6.9.69'
            - yandex-hw-watcher-shelf-tool-plugin: 1.0
            - mcelog: 157+dfsg-1yandex1
        - prereq_in:
            - cmd: repositories-ready

/etc/hw_watcher/hw_watcher.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/hw_watcher.conf' }}
        - require:
            - pkg: hw-watcher-pkg
{% endif %}

include:
    - components.monrun2.hw-watcher
