include:
    - components.repositories.apt.yandex-cauth
{% if salt.pillar.get('data:use_monrun', True) and salt.pillar.get('data:cauth_use_v2') %}
    - components.monrun2.cauth
{% endif %}

/etc/cauth/cauth.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/cauth.conf
        - mode: 644

{% if salt.pillar.get('data:cauth_use_v2') %}
yandex-cauth:
    pkg.purged

yandex-cauth-userd:
    pkg.installed:
        - version: '2.1-9536319'
        - require:
            - pkg: yandex-cauth
            - cmd: repositories-ready
        # yandex-cauth-userd modify /etc/ssh/sshd_config in postinst,
        # so it's better to modify config after it
        - require_in:
            - file: /etc/ssh/sshd_config
            - file: /etc/cauth/cauth.conf

yandex-cauth-userd-service:
    service.running:
        - name: yandex-cauth-userd
        - enable: True
        - watch:
              - pkg: yandex-cauth-userd
              - file: /etc/cauth/cauth.conf

{% else %}
yandex-cauth-userd:
    pkg.purged

remove-cauth-client-caching:
    pkg.purged:
        - pkgs:
            - cauth-client-scripts
            - cauth-client-caching
        - onlyif:
            - "dpkg -l | fgrep -q cauth-client"

yandex-cauth:
    pkg.installed:
        - version: '1.6.3'
        - require:
            - cmd: repositories-ready
            - pkg: yandex-cauth-userd
            - pkg: remove-cauth-client-caching
        - require_in:
            - file: /etc/cauth/cauth.conf
{% endif %}
