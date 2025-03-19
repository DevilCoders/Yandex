{% set osrelease = salt.grains.get('osrelease') %}

jaeger-agent-pkgs:
    pkg.installed:
        - pkgs:
            - jaeger-agent: '1.7769594'

/etc/jaeger/agent.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/agent.yaml' }}
        - mode: '0640'
        - user: root
        - group: www-data
        - makedirs: True
        - require:
            - pkg: jaeger-agent-pkgs

{% if osrelease == '18.04' %}
/lib/systemd/system/jaeger-agent.service:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/jaeger-agent.service' }}
        - mode: '0644'
        - user: root
        - group: www-data
        - makedirs: True
        - require:
            - pkg: jaeger-agent-pkgs
{% else %}
/etc/init/jaeger-agent.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/jaeger-agent.upstart' }}
        - mode: '0640'
        - user: root
        - group: www-data
        - makedirs: True
        - require:
            - pkg: jaeger-agent-pkgs
{% endif %}

jaeger-agent-service:
    service.running:
        - name: jaeger-agent
        - enable: True
        - watch:
            - pkg: jaeger-agent-pkgs
            - file: /etc/jaeger/agent.yaml
{% if osrelease == '18.04' %}
            - file: /lib/systemd/system/jaeger-agent.service
{% else %}
            - file: /etc/init/jaeger-agent.conf
{% endif %}
