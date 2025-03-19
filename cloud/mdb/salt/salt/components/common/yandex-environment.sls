{% if salt['pillar.get']('yandex-environment:name') or salt['pillar.get']('yandex-environment:type') %}
yandex-environment-dummy-package:
    pkg.installed:
        - pkgs:
            - yandex-environment-dummy
{% endif %}
{% if salt['pillar.get']('yandex-environment:name') %}
yandex-environment-name-file:
    file.managed:
        - name: /etc/yandex/environment.name
        - source: salt://components/common/conf/etc/yandex/environment.name
        - makedirs: True
        - user: root
        - group: root
        - mode: 644
        - template: jinja
        - require:
            - pkg: yandex-environment-dummy-package
yandex-environment-name-xml-file:
    file.managed:
        - name: /etc/yandex/environment.name.xml
        - source: salt://components/common/conf/etc/yandex/environment.name.xml
        - makedirs: True
        - user: root
        - group: root
        - mode: 644
        - template: jinja
        - require:
            - pkg: yandex-environment-dummy-package
{% endif %}

{% if salt['pillar.get']('yandex-environment:type') %}
yandex-environment-type-file:
    file.managed:
        - name: /etc/yandex/environment.type
        - source: salt://components/common/conf/etc/yandex/environment.type
        - makedirs: True
        - user: root
        - group: root
        - mode: 644
        - template: jinja
        - require:
            - pkg: yandex-environment-dummy-package
yandex-environment-type-xml-file:
    file.managed:
        - name: /etc/yandex/environment.type.xml
        - source: salt://components/common/conf/etc/yandex/environment.type.xml
        - makedirs: True
        - user: root
        - group: root
        - mode: 644
        - template: jinja
        - require:
            - pkg: yandex-environment-dummy-package
{% endif %}

